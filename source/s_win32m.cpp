//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2001 J�nis Legzdi��
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**	$Log$
//**	Revision 1.2  2001/07/27 14:27:54  dj_jl
//**	Update with Id-s and Log-s, some fixes
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "winlocal.h"
#include <dmusici.h>

#include "gamedefs.h"
#include "s_local.h"

// MACROS ------------------------------------------------------------------

#define MIDIMAGIC    		"MThd"
#define MUSMAGIC     		"MUS\032"

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int qmus2mid(char*, char*, int);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static IDirectMusicPerformance*	Performance = NULL;
static IDirectMusicLoader*		Loader = NULL;
static IDirectMusicSegment*		Segment = NULL;
static MUSIC_TIME				StartTime;
static MUSIC_TIME				TimeOffset;
static MUSIC_TIME				SegmentLength;

static boolean			enabled = false;
static boolean			music_started = false;

static char				Mus_Song[9] = "";
static boolean			mus_looping = false;
static void*			Mus_SndPtr  = NULL;
static boolean			MusicPaused = false;
static int				MusVolume = -1;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	S_InitMusic
//
//==========================================================================

void S_InitMusic(void)
{
}

//==========================================================================
//
//	S_InitDirectMusic
//
//==========================================================================

void S_InitDirectMusic(LPDIRECTSOUND DSound)
{
	HRESULT			result;

	if (M_CheckParm("-nomusic"))
	{
		return;
	}

	result = CoCreateInstance(CLSID_DirectMusicPerformance, NULL,
		CLSCTX_INPROC, IID_IDirectMusicPerformance2, (void**)&Performance);
	if (result != S_OK)
		Sys_Error("S_InitMusic: Failed to create performance");

	result = Performance->Init(NULL, DSound, NULL);
	if (result != S_OK)
		Sys_Error("S_InitMusic: Failed to initialize performance");

	result = Performance->AddPort(NULL);
	if (result != S_OK)
		Sys_Error("S_InitMusic: Failed to add port to Performance");

	result = CoCreateInstance(CLSID_DirectMusicLoader, NULL,
		CLSCTX_INPROC, IID_IDirectMusicLoader, (void**)&Loader);
	if (result != S_OK)
		Sys_Error("S_InitMusic: Failed to create music loader");

	music_started = true;
	enabled = true;
}

//==========================================================================
//
//	S_ShutdownMusic
//
//==========================================================================

void S_ShutdownMusic(void)
{
	if (music_started)
	{
		// Release segment
		if (Segment)
			Segment->Release();
		Segment = NULL;

		// Release the loader object.    
		if (Loader)
			Loader->Release();
		Loader = NULL;

		// CloseDown and Release the performance object.    
		if (Performance)
		{
			Performance->CloseDown();
			Performance->Release();
		}
		Performance = NULL;
		music_started = false;
	}
}

//==========================================================================
//
//	PlaySong
//
//==========================================================================

static void PlaySong(void* data, int length, int looping)
{
    DMUS_OBJECTDESC			ObjDesc; 
    HRESULT					result;

	if (strncmp((char*)data, "MThd", 4))
	{
		con << "PlaySong: Bad MIDI header\n";
		return;
	}

    ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
    ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
    ObjDesc.guidClass = CLSID_DirectMusicSegment;
	ObjDesc.pbMemData = (byte*)data;
	ObjDesc.llMemLength = length;

    result = Loader->GetObject(&ObjDesc, IID_IDirectMusicSegment2, (void**)&Segment);
	if (result != S_OK)
	{
		con << "PlaySong: Failed to get object\n";
		return;
	}

    result = Segment->SetParam(GUID_StandardMIDIFile, -1, 0, 0, (void*)Performance);
	if (result != S_OK)
		Sys_Error("PlaySong: Failed to set param");

    result = Segment->SetParam(GUID_Download, -1, 0, 0, (void*)Performance);
	if (result != S_OK)
		Sys_Error("PlaySong: Failed to download segment");

	if (looping)
		Segment->SetRepeats(1000000);

	if (Performance->PlaySegment(Segment, 0, 0, NULL) != S_OK)
		con << "PlaySegment failed\n";

    Performance->GetTime(NULL, &StartTime);
	TimeOffset = 0;
	Segment->GetLength(&SegmentLength);
}

//==========================================================================
//
//	PauseSong
//
//==========================================================================

static void PauseSong(void)
{
	if (!Segment)
		return;

	Performance->Stop(Segment, NULL, 0, 0);

    MUSIC_TIME          NowTime;
                
    //Find out the current performance time so that we can figure out 
    //where we stopped in the segment.
    Performance->GetTime(NULL, &NowTime);

    // Caculate the offset into the segment in music time (ticks)
    // and add to previous offset in 
    // cause there has been more than one pause in this segment playback
    TimeOffset = ((NowTime - StartTime) + TimeOffset) % SegmentLength;

    // Set restart point
    Segment->SetStartPoint(TimeOffset);
}

//==========================================================================
//
//	ResumeSong
//
//==========================================================================

static void ResumeSong(void)
{
	if (!Segment)
		return;

	Performance->PlaySegment(Segment, 0, 0, NULL);
    Performance->GetTime(NULL, &StartTime);
}

//==========================================================================
//
//  S_UpdateMusic
//
//==========================================================================

void S_UpdateMusic(void)
{
	if (!music_started)
    	return;

	//	Update volume
    if ((int)music_volume < 0 )
    {
    	music_volume = 0;
    }
    if (music_volume > 15)
    {
    	music_volume = 15;
    }

	if (music_volume != MusVolume)
    {
		HRESULT		res;
		int			vol;

		if (!MusVolume && (int)music_volume && !MusicPaused)
	    {
		   	ResumeSong();
	    }
		if (MusVolume && !(int)music_volume)
	    {
		   	PauseSong();
	    }
		MusVolume = music_volume;

		vol = (MusVolume - 15) * 300;
		res = Performance->SetGlobalParam(GUID_PerfMasterVolume, &vol, sizeof(long));
		if (res != S_OK)
		{
			if (res == E_FAIL)
				con << "Failed to set music volume\n";
			else if (res == E_POINTER)
				con << "Invalid pointer\n";
			else if (res == E_OUTOFMEMORY)
				con << "Out of memory\n";
			else
				con << "Invalid error\n";
		}
    }
}

//==========================================================================
//
//	QrySongPlaying
//
//	Is the song playing?
//
//==========================================================================

static int QrySongPlaying(void)
{
	if (!Segment)
		return 0;

	return Performance->IsPlaying(Segment, NULL) == S_OK;
}

//==========================================================================
//
//	StopSong
//
//==========================================================================

static void StopSong(void)
{
	if (!Segment)
		return;

	Performance->Stop(Segment, NULL,0, 0);

	Segment->Release();
	Segment = NULL;
}

//==========================================================================
//
//  StopMidiSong
//
//==========================================================================

static void StopMidiSong(void)
{
	if (Mus_SndPtr)
	{
		if (MusicPaused)
        {
			ResumeSong();
		}
		StopSong();
		if (UseSndScript)
		{
			Z_Free(Mus_SndPtr);
		}
		else
		{
			Z_ChangeTag(Mus_SndPtr, PU_CACHE);
		}
		Mus_SndPtr = NULL;
        Mus_Song[0] = 0;
	}
}

//==========================================================================
//
//	StartMidiSong
//
//==========================================================================

static void StartMidiSong(char* song, boolean loop)
{
	int			len;
    char		*buf;

	if (!song || !song[0])
	{
       	return;
	}

	if (!stricmp(song, Mus_Song) && QrySongPlaying())
	{
       	// don't replay an old song if it is still playing
		return;
	}

	if (UseSndScript)
	{
		char	*name;

		name = va("%s%s.lmp", ArchivePath, song);
	    if (!Sys_FileExists(name))
	    {
			name = va("%s%s.mid", ArchivePath, song);
		    if (!Sys_FileExists(name))
		    {
		    	con << "Can't find song \"" << song << "\"\n";
		        return;
			}
	    }
		StopMidiSong();
		len = M_ReadFile(name, (byte **)&Mus_SndPtr);
	}
	else
	{
		int		lump;

		lump = W_CheckNumForName(song);
    	if (lump < 0)
	    {
	    	con << "Can't find song \"" << song << "\"\n";
    	    return;
	    }
		StopMidiSong();
		Mus_SndPtr = W_CacheLumpNum(lump, PU_MUSIC);
		len = W_LumpLength(lump);
	}

	if (!memcmp(Mus_SndPtr, MUSMAGIC, 4))
	{
   		// convert mus to mid with a wanderfull function
		// thanks to S.Bacquet for the source of qmus2mid
		buf = (char*)Z_Malloc(256 * 1024, PU_STATIC, 0);
        len = qmus2mid((char*)Mus_SndPtr, buf, len);
        if (!len)
        {
	        Z_Free(buf);
        	return;
        }
    	Z_Resize((void**)&Mus_SndPtr, len);
        memcpy(Mus_SndPtr, buf, len);
        Z_Free(buf);
	}

	PlaySong(Mus_SndPtr, len, loop); // 'true' denotes endless looping.
	if (!MusVolume || MusicPaused)
	{
		PauseSong();
	}
	strcpy(Mus_Song, song);
    mus_looping = loop;
}

//==========================================================================
//
//  Music_f
//
//==========================================================================

COMMAND(Music)
{
	char	*command;

	if (!music_started)
    {
    	return;
    }

	S_UpdateMusic();

	if (Argc() < 2)
	{
		return;
	}

	command = Argv(1);

	if (!stricmp(command, "on"))
	{
		enabled = true;
		return;
	}

	if (!stricmp(command, "off"))
	{
		StopMidiSong();
		enabled = false;
		return;
	}

	if (!enabled)
    {
    	return;
    }

	if (!stricmp(command, "play"))
    {
		StartMidiSong(Argv(2), false);
		return;
	}

	if (!stricmp(command, "loop"))
    {
		StartMidiSong(Argv(2), true);
		return;
	}

	if (!stricmp(command, "pause"))
    {
		PauseSong();
		MusicPaused = true;
		return;
    }

	if (!stricmp(command, "resume"))
    {
		ResumeSong();
		MusicPaused = false;
		return;
    }

	if (!stricmp(command, "stop"))
    {
    	StopMidiSong();
		return;
    }

	if (!stricmp(command, "playing"))
	{
	    if (QrySongPlaying())
        {
        	con << "Currently " << (mus_looping ? "looping" : "playing")
        		<< " \"" << Mus_Song << "\".\n";
        }
        else
        {
        	con << "No song currently playing\n";
        }
        return;
	}
}


//**************************************************************************
//**
//**	qmus2mid.c
//**
//**    Quick MUS->MID ! by S.Bacquet
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

// MACROS ------------------------------------------------------------------

#define MUSMAGIC     		"MUS\032"
#define MIDIMAGIC    		"MThd"

#define last(e) 			((byte)(e & 0x80))
#define event_type(e)		((byte)((e & 0x7F) >> 4))
#define channel(e)			((byte)(e & 0x0F))

#define TRACKBUFFERSIZE		65536  /* 64 Ko */

// TYPES -------------------------------------------------------------------

#pragma pack(1)

typedef struct
{
	char		ID[4];			/* identifier "MUS" 0x1A */
	word		ScoreLength;
	word		ScoreStart;
	word		channels; 		/* count of primary channels */
	word		SecChannels;	/* count of secondary channels (?) */
	word		InstrCnt;
	word		dummy;
} MUSheader;

typedef struct
{
	char		ID[4];
    dword		size;
    word		file_type;
	word		num_tracks;
	word		divisions;
} MIDI_Header_t;

#pragma pack()

struct Track
{
	dword		current;
	char	 	vel;
	long	 	DeltaTime;
	byte		LastEvent;
	char		*data; 	   /* Primary data */
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char				*mid_file;

static struct Track		tracks[32];
static word				TrackCnt = 0;
static int	 			MUS2MIDchannel[16];

static byte				track_end[] =
	{0x00, 0xff, 47, 0x00};
static byte				midikey[] =
	{0x00, 0xff, 0x59, 0x02, 0x00, 0x00};   	// C major
static byte				miditempo[] =
	{0x00, 0xff, 0x51, 0x03, 0x09, 0xa3, 0x1a};	// uS/qnote

// CODE --------------------------------------------------------------------

//==========================================================================
//
//  FirstChannelAvailable
//
//==========================================================================

static int FirstChannelAvailable(void)
{
	int 	i;
	int 	old15 = MUS2MIDchannel[15];
	int		max = -1;

	MUS2MIDchannel[15] = -1;
	for (i = 0; i < 16; i++)
    	if (MUS2MIDchannel[i] > max)
    		max = MUS2MIDchannel[i];
	MUS2MIDchannel[15] = old15 ;

	return (max == 8 ? 10 : max + 1);
}

//==========================================================================
//
//  TWriteByte
//
//==========================================================================

static void TWriteByte(int MIDItrack, char data)
{
	if (tracks[MIDItrack].current < TRACKBUFFERSIZE)
    {
    	tracks[MIDItrack].data[tracks[MIDItrack].current] = data;
	}
	else
    {
		Sys_Error("qmus2mid: Track buffer full.");
    }
	tracks[MIDItrack].current++;
}

//==========================================================================
//
//	TWriteBuf
//
//==========================================================================

static void TWriteBuf(int MIDItrack, byte *buf, int size)
{
	int		i;

	for (i = 0; i < size; i++)
    {
    	TWriteByte(MIDItrack, buf[i]);
    }
}

//==========================================================================
//
//  TWriteVarLen
//
//==========================================================================

static void TWriteVarLen(int tracknum, register dword value)
{
	register dword	buffer ;

	buffer = value & 0x7f ;
	while( (value >>= 7) )
    {
      	buffer <<= 8 ;
      	buffer |= 0x80 ;
      	buffer += (value & 0x7f) ;
    }
  	while (1)
    {
		TWriteByte(tracknum, (byte)(buffer & 0xff));
      	if (buffer & 0x80)
			buffer >>= 8 ;
      	else
			break;
    }
}

//==========================================================================
//
//  ReadTime
//
//==========================================================================

static dword ReadTime(char **file)
{
	register dword 	time = 0 ;
	int 			data;

	do
    {
    	data = *(*file)++;
		if (data != EOF)
			time = (time << 7) + (data & 0x7F);
    } while ((data != EOF) && (data & 0x80));

	return time;
}

//==========================================================================
//
//  convert
//
//==========================================================================

static boolean convert(char *mus, int length)
{
	char				*mus_ptr;
	byte				et;
	int					MUSchannel;
	int					MIDIchannel;
	int					MIDItrack = 0;
	int					NewEvent;
	int 				i;
	int					event;
	int					data;
	dword				DeltaTime;
	dword				TotalTime = 0;
	byte				MUS2MIDcontrol[15] =
	{
	    0,				/* Program change - not a MIDI control change */
	    0x00,			/* Bank select */
	    0x01,			/* Modulation pot */
	    0x07,			/* Volume */
	    0x0A,			/* Pan pot */
	    0x0B,			/* Expression pot */
	    0x5B,			/* Reverb depth */
	    0x5D,			/* Chorus depth */
	    0x40,			/* Sustain pedal */
	    0x43,			/* Soft pedal */
	    0x78,			/* All sounds off */
	    0x7B,			/* All notes off */
	    0x7E,			/* Mono */
	    0x7F,			/* Poly */
	    0x79			/* Reset all controllers */
	};
	byte				MIDIchan2track[16];
  	char 				ouch = 0;
	MUSheader			*MUSh;

	for (i = 0 ; i < 16 ; i++)
	{
		MUS2MIDchannel[i] = -1;
	}
	for (i = 0 ; i < 32 ; i++)
	{
      	tracks[i].current = 0;
      	tracks[i].vel = 64;
      	tracks[i].DeltaTime = 0;
      	tracks[i].LastEvent = 0;
      	tracks[i].data = NULL;
	}

	MUSh = (MUSheader *)mus;
	if (strncmp(MUSh->ID, MUSMAGIC, 4))
    {
    	con << "Not a MUS file\n";
		return false;
	}

	if (MUSh->channels > 15)	 /* <=> MUSchannels+drums > 16 */
    {
    	cond << "Too many channels\n";
		return false;
	}

	mus_ptr = mus + MUSh->ScoreStart;

	tracks[0].data = (char *)Z_Malloc(TRACKBUFFERSIZE, PU_MUSIC, 0);
	TWriteBuf(0, midikey, 6);
	TWriteBuf(0, miditempo, 7);

	TrackCnt = 1;	//	Music starts here

	event = *(mus_ptr++);
	et = event_type(event);
	MUSchannel = channel(event);
	while ((et != 6) && mus_ptr - mus < length && (event != EOF))
    {
		if (MUS2MIDchannel[MUSchannel] == -1)
		{
	  		MIDIchannel = MUS2MIDchannel[MUSchannel] =
	    		(MUSchannel == 15 ? 9 : FirstChannelAvailable());
	  		MIDItrack   = MIDIchan2track[MIDIchannel] = (byte)TrackCnt++;
			tracks[MIDItrack].data = (char *)Z_Malloc(TRACKBUFFERSIZE, PU_MUSIC, 0);
		}
      	else
		{
	  		MIDIchannel = MUS2MIDchannel[MUSchannel];
	  		MIDItrack   = MIDIchan2track[MIDIchannel];
		}
      	TWriteVarLen(MIDItrack, tracks[MIDItrack].DeltaTime);
      	tracks[MIDItrack].DeltaTime = 0 ;
      	switch (et)
		{
		 case 0:		/* release note */
  			NewEvent = 0x90 | MIDIchannel;
   			TWriteByte(MIDItrack, NewEvent);
   			tracks[MIDItrack].LastEvent = NewEvent;
  			data = *(mus_ptr++);
  			TWriteByte(MIDItrack, data);
  			TWriteByte(MIDItrack, 0);
  			break;
		 case 1:
  			NewEvent = 0x90 | MIDIchannel;
   			TWriteByte(MIDItrack, NewEvent);
   			tracks[MIDItrack].LastEvent = NewEvent;
  			data = *(mus_ptr++);
  			TWriteByte(MIDItrack, data & 0x7F);
  			if (data & 0x80)
    			tracks[MIDItrack].vel = *(mus_ptr++);
  			TWriteByte(MIDItrack, tracks[MIDItrack].vel);
  			break;
		 case 2:
  			NewEvent = 0xE0 | MIDIchannel;
   			TWriteByte(MIDItrack, NewEvent);
   			tracks[MIDItrack].LastEvent = NewEvent;
  			data = *(mus_ptr++);
  			TWriteByte(MIDItrack, (data & 1) << 6);
  			TWriteByte(MIDItrack, data >> 1);
  			break;
		 case 3:
  			NewEvent = 0xB0 | MIDIchannel;
   			TWriteByte(MIDItrack, NewEvent);
   			tracks[MIDItrack].LastEvent = NewEvent;
  			data = *(mus_ptr++) ;
  			TWriteByte(MIDItrack, MUS2MIDcontrol[data]);
  			if (data == 12)
    			TWriteByte(MIDItrack, MUSh->channels + 1);
  			else
    			TWriteByte(MIDItrack, 0);
  			break;
		 case 4:
  			data = *(mus_ptr++);
  			if (data)
    		{
      			NewEvent = 0xB0 | MIDIchannel;
  				TWriteByte(MIDItrack, NewEvent);
  				tracks[MIDItrack].LastEvent = NewEvent;
      			TWriteByte(MIDItrack, MUS2MIDcontrol[data]);
    		}
  			else
    		{
      			NewEvent = 0xC0 | MIDIchannel;
  				TWriteByte(MIDItrack, NewEvent);
  				tracks[MIDItrack].LastEvent = NewEvent;
    		}
  			data = *(mus_ptr++);
  			TWriteByte(MIDItrack, data);
  			break;
		 case 5:
		 case 7:
			cond << "MUS file corupted\n";
  			return false;
		 default:
           	break;
		}
      	if (last(event))
		{
	  		DeltaTime = ReadTime(&mus_ptr);
	  		TotalTime += DeltaTime;
	  		for (i = 0; i < (int)TrackCnt; i++)
	    		tracks[i].DeltaTime += DeltaTime;
		}
      	event = *(mus_ptr++);
      	if (event != EOF)
      	{
	  		et = event_type(event);
	  		MUSchannel = channel(event);
		}
      	else
        {
			ouch = 1;
		}
	}

    for (i = 0; i < TrackCnt; i++)
    {
		TWriteBuf(i, track_end, 4);
	}

	if (ouch)
    {
		cond << "WARNING : There are bytes missing at the end.\n"
		"The end of the MIDI file might not fit the original one.\n";
	}

	return true;
}

//==========================================================================
//
//  WriteBuf
//
//==========================================================================

static void WriteBuf(void *p, int size)
{
	memcpy(mid_file, p, size);
	mid_file += size;
}

//==========================================================================
//
//	WriteMIDIFile
//
//==========================================================================

static void WriteMIDIFile(void)
{
	int				i;
	dword			size;
	MIDI_Header_t	*hdr;

	//	Header
	hdr = (MIDI_Header_t*)mid_file;
	memcpy(hdr->ID, MIDIMAGIC, 4);
    hdr->size       = BigLong(6);
    hdr->file_type  = BigShort(1);
	hdr->num_tracks = BigShort(TrackCnt);
	hdr->divisions  = BigShort(89);
	mid_file += sizeof(*hdr);

	//	Tracks
	for (i = 0; i < (int)TrackCnt; i++)
    {
		size = BigLong(tracks[i].current);
		WriteBuf("MTrk", 4);
		WriteBuf(&size, 4);
		WriteBuf(tracks[i].data, tracks[i].current);
	}
}

//==========================================================================
//
//  FreeTracks
//
//==========================================================================

static void FreeTracks(void)
{
	int i ;

	for (i = 0; i < 16; i++)
    {
    	if (tracks[i].data)
        {
      		Z_Free(tracks[i].data);
            tracks[i].data = NULL;
		}
	}
}

//==========================================================================
//
//  qmus2mid
//
//==========================================================================

static int qmus2mid(char *mus, char *mid, int length)
{
	mid_file = mid;

	if (convert(mus, length))
    {
	    WriteMIDIFile();
	}
	FreeTracks();

    return mid_file - mid;
}
