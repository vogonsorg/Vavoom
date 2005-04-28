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
//**	Copyright (C) 1999-2002 J�nis Legzdi��
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
//**************************************************************************
//**
//**	Preparation of data for rendering, generation of lookups, caching,
//**  retrieval by name.
//**
//** 	Graphics.
//**
//** 	DOOM graphics for walls and sprites is stored in vertical runs of
//**  opaque pixels (posts). A column is composed of zero or more posts, a
//**  patch or sprite is composed of zero or more columns.
//**
//** 	Texture definition.
//**
//** 	Each texture is composed of one or more patches, with patches being
//**  lumps stored in the WAD. The lumps are referenced by number, and
//**  patched into the rectangular texture space using origin and possibly
//**  other attributes.
//**
//** 	Texture definition.
//**
//** 	A DOOM wall texture is a list of patches which are to be combined in
//**  a predefined order.
//**
//** 	A single patch from a texture definition, basically a rectangular
//**  area within the texture rectangle.
//**
//** 	A maptexturedef_t describes a rectangular texture, which is composed
//**  of one or more mappatch_t structures that arrange graphic patches.
//**
//** 	MAPTEXTURE_T CACHING
//**
//** 	When a texture is first needed, it counts the number of composite
//**  columns required in the texture and allocates space for a column
//**  directory and any new columns. The directory will simply point inside
//**  other patches if there is only one patch in a given column, but any
//**  columns with multiple patches will have new column_ts generated.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"
#include "ftexdefs.h"
#include "r_local.h"

// MACROS ------------------------------------------------------------------

#define ANIM_SCRIPT_NAME	"ANIMDEFS"
#define ANIM_FLAT 			0
#define ANIM_TEXTURE 		1
#define SCI_FLAT 			"flat"
#define SCI_TEXTURE 		"texture"
#define SCI_PIC 			"pic"
#define SCI_TICS 			"tics"
#define SCI_RAND 			"rand"

// TYPES -------------------------------------------------------------------

struct frameDef_t
{
	int index;
	short baseTime;
	short randomRange;
};

struct animDef_t
{
	int type;
	int index;
	float time;
	int currentFrameDef;
	int startFrameDef;
	int endFrameDef;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

//
// Texture data
//
int				numtextures;
texdef_t**		textures;
int*			texturetranslation; // Animation

//
// Flats data
//
int				numflats;
int*			flatlumps;
int*			flattranslation;    // Animation
int				skyflatnum;			// sky mapping

//
// Sprite lumps data
//
int				numspritelumps;
int*			spritelumps;
int*			spritewidth;		// needed for pre rendering
int*			spriteheight;
int*			spriteoffset;
int*			spritetopoffset;

//
//	Translation tables
//
byte*			translationtables;

//
//	2D graphics
//
pic_info_t		pic_list[MAX_PICS];

rgba_t			r_palette[MAX_PALETTES][256];
byte			r_black_color[MAX_PALETTES];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static TArray<animDef_t>	AnimDefs;
static TArray<frameDef_t>	FrameDefs;

static float*				textureheight;	// needed for texture pegging

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	IsStrifeTexture
//
//==========================================================================

static bool IsStrifeTexture(void)
{
	guard(IsStrifeTexture);
	int *plump = (int*)W_CacheLumpName("TEXTURE1", PU_STATIC);
	int numtex = LittleLong(*plump);
	int *texdir = plump + 1;
	int i;
	for (i = 0; i < numtex - 1; i++)
	{
		if (LittleLong(texdir[i + 1]) - LittleLong(texdir[i]) == sizeof(maptexture_strife_t))
		{
			break;
		}
	}
	Z_Free(plump);
	return i != numtex - 1;
	unguard;
}

//==========================================================================
//
//	InitTextures
//
// 	Initializes the texture list with the textures from the world map.
//
//==========================================================================

static void InitTextures(void)
{
	guard(InitTextures);
	maptexture_t*	mtexture;
	texdef_t*		texture;
	texpatch_t*		patch;

    int				i;
    int				j;

    int*			maptex;
    int*			maptex2;
    int*			maptex1;
    
    char			name[9];
    char*			names;
    char*			name_p;
    
    int*			patchlookup;
    
    int				nummappatches;
    int				offset;
    int				maxoff;
    int				maxoff2;
    int				numtextures1;
    int				numtextures2;

    int*			directory;
    
    // Load the patch names from pnames.lmp.
    name[8] = 0;
    names = (char*)W_CacheLumpName("PNAMES", PU_STATIC);
    nummappatches = LittleLong(*((int *)names));
    name_p = names + 4;
    patchlookup = (int*)Z_Malloc(nummappatches*sizeof(*patchlookup), PU_HIGH, 0);

    for (i = 0; i < nummappatches; i++)
	{
		strncpy(name, name_p + i * 8, 8);
		patchlookup[i] = W_CheckNumForName(name);
		//	Sprites also can be used as patches.
		if (patchlookup[i] < 0)
			patchlookup[i] = W_CheckNumForName(name, WADNS_Sprites);
	}
    Z_Free(names);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex = maptex1 = (int*)W_CacheLumpName("TEXTURE1", PU_HIGH);
    numtextures1 = LittleLong(*maptex);
    maxoff = W_LumpLength(W_GetNumForName("TEXTURE1"));
    directory = maptex+1;
	
    if (W_CheckNumForName("TEXTURE2") != -1)
    {
		maptex2 = (int*)W_CacheLumpName("TEXTURE2", PU_HIGH);
		numtextures2 = LittleLong(*maptex2);
		maxoff2 = W_LumpLength(W_GetNumForName ("TEXTURE2"));
    }
    else
    {
		maptex2 = NULL;
		numtextures2 = 0;
		maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = (texdef_t**)Z_Calloc(numtextures * 4);
    textureheight = (float*)Z_Calloc(numtextures * 4);
    texturetranslation = (int*)Z_Calloc((numtextures + 1) * 4);

    for (i=0 ; i<numtextures ; i++, directory++)
    {
		if (i == numtextures1)
		{
	    	// Start looking in second texture file.
	    	maptex = maptex2;
	    	maxoff = maxoff2;
	    	directory = maptex+1;
		}
		
		offset = LittleLong(*directory);

		if (offset > maxoff)
		{
	    	Sys_Error("InitTextures: bad texture directory");
		}
	
		mtexture = (maptexture_t*)((byte *)maptex + offset);

		texture = textures[i] =
	    	(texdef_t*)Z_Malloc(sizeof(texdef_t)
		      + sizeof(texpatch_t) * (LittleShort(mtexture->patchcount) - 1),
		      PU_STATIC, 0);
	
		texture->width = LittleShort(mtexture->width);
		texture->height = LittleShort(mtexture->height);
		texture->patchcount = LittleShort(mtexture->patchcount);
		texture->SScale = mtexture->sscale ? mtexture->sscale / 8.0 : 1.0;
		texture->TScale = mtexture->tscale ? mtexture->tscale / 8.0 : 1.0;

		memcpy(texture->name, mtexture->name, sizeof(texture->name));
		patch = texture->patches;

		for (j = 0; j < texture->patchcount; j++, patch++)
		{
	    	patch->originx = LittleShort(mtexture->patches[j].originx);
	    	patch->originy = LittleShort(mtexture->patches[j].originy);
	    	patch->patch = patchlookup[LittleShort(mtexture->patches[j].patch)];
	    	if (patch->patch == -1)
	    	{
				Sys_Error("InitTextures: Missing patch in texture %s",
			 		texture->name);
	    	}
		}

		//	Fix sky texture heights for Heretic, but it can also be used
		// for Doom and Strife
		if (!strnicmp(texture->name, "SKY", 3) && texture->height == 128)
		{
			patch_t *realpatch = (patch_t*)W_CacheLumpNum(
				texture->patches[0].patch, PU_TEMP);
			if (LittleShort(realpatch->height) > texture->height)
			{
				texture->height = LittleShort(realpatch->height);
			}
		}

		textureheight[i] = texture->height / texture->TScale;
		
    	// Create translation table for global animation.
		texturetranslation[i] = i;
    }

	Z_Free(patchlookup);
    Z_Free(maptex1);
    if (maptex2)
		Z_Free(maptex2);
	unguard;
}

//==========================================================================
//
//	InitTextures2
//
// 	Initializes the texture list with the textures from the world map.
//	Strife texture format version.
//
//==========================================================================

static void InitTextures2(void)
{
	guard(InitTextures);
	maptexture_strife_t	*mtexture;
	texdef_t*			texture;
	texpatch_t*			patch;

    int				i;
    int				j;

    int*			maptex;
    int*			maptex2;
    int*			maptex1;
    
    char			name[9];
    char*			names;
    char*			name_p;
    
    int*			patchlookup;
    
    int				nummappatches;
    int				offset;
    int				maxoff;
    int				maxoff2;
    int				numtextures1;
    int				numtextures2;

    int*			directory;
    
    // Load the patch names from pnames.lmp.
    name[8] = 0;
    names = (char*)W_CacheLumpName("PNAMES", PU_STATIC);
    nummappatches = LittleLong(*((int *)names));
    name_p = names + 4;
    patchlookup = (int*)Z_Malloc(nummappatches*sizeof(*patchlookup), PU_HIGH, 0);

    for (i = 0; i < nummappatches; i++)
	{
		strncpy(name, name_p + i * 8, 8);
		patchlookup[i] = W_CheckNumForName(name);
		//	Sprites also can be used as patches.
		if (patchlookup[i] < 0)
			patchlookup[i] = W_CheckNumForName(name, WADNS_Sprites);
	}
    Z_Free(names);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex = maptex1 = (int*)W_CacheLumpName("TEXTURE1", PU_HIGH);
    numtextures1 = LittleLong(*maptex);
    maxoff = W_LumpLength(W_GetNumForName("TEXTURE1"));
    directory = maptex+1;
	
    if (W_CheckNumForName("TEXTURE2") != -1)
    {
		maptex2 = (int*)W_CacheLumpName("TEXTURE2", PU_HIGH);
		numtextures2 = LittleLong(*maptex2);
		maxoff2 = W_LumpLength(W_GetNumForName ("TEXTURE2"));
    }
    else
    {
		maptex2 = NULL;
		numtextures2 = 0;
		maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = (texdef_t**)Z_Calloc(numtextures * 4);
    textureheight = (float*)Z_Calloc(numtextures * 4);
    texturetranslation = (int*)Z_Calloc((numtextures + 1) * 4);

    for (i=0 ; i<numtextures ; i++, directory++)
    {
		if (i == numtextures1)
		{
	    	// Start looking in second texture file.
	    	maptex = maptex2;
	    	maxoff = maxoff2;
	    	directory = maptex+1;
		}
		
		offset = LittleLong(*directory);

		if (offset > maxoff)
		{
	    	Sys_Error("InitTextures: bad texture directory");
		}
	
		mtexture = (maptexture_strife_t*)((byte *)maptex + offset);

		texture = textures[i] =
	    	(texdef_t*)Z_Malloc(sizeof(texdef_t)
		      + sizeof(texpatch_t) * (LittleShort(mtexture->patchcount) - 1),
		      PU_STATIC, 0);
	
		texture->width = LittleShort(mtexture->width);
		texture->height = LittleShort(mtexture->height);
		texture->patchcount = LittleShort(mtexture->patchcount);
		texture->SScale = 1.0;
		texture->TScale = 1.0;

		memcpy(texture->name, mtexture->name, sizeof(texture->name));
		patch = texture->patches;

		for (j = 0; j < texture->patchcount; j++, patch++)
		{
	    	patch->originx = LittleShort(mtexture->patches[j].originx);
	    	patch->originy = LittleShort(mtexture->patches[j].originy);
	    	patch->patch = patchlookup[LittleShort(mtexture->patches[j].patch)];
	    	if (patch->patch == -1)
	    	{
				Sys_Error("InitTextures: Missing patch in texture %s",
			 		texture->name);
	    	}
		}

		//	Fix sky texture heights for Heretic, but it can also be used
		// for Doom and Strife
		if (!strnicmp(texture->name, "SKY", 3) && texture->height == 128)
		{
			patch_t *realpatch = (patch_t*)W_CacheLumpNum(
				texture->patches[0].patch, PU_TEMP);
			if (LittleShort(realpatch->height) > texture->height)
			{
				texture->height = LittleShort(realpatch->height);
			}
		}

		textureheight[i] = texture->height;
		
    	// Create translation table for global animation.
		texturetranslation[i] = i;
    }

	Z_Free(patchlookup);
    Z_Free(maptex1);
    if (maptex2)
		Z_Free(maptex2);
	unguard;
}

//==========================================================================
//
//  R_CheckTextureNumForName
//
// 	Check whether texture is available. Filter out NoTexture indicator.
//
//==========================================================================

int	R_CheckTextureNumForName(const char *name)
{
	guard(R_CheckTextureNumForName);
    int		i;

    // "NoTexture" marker.
    if (name[0] == '-')		
		return 0;
		
    for (i=0 ; i<numtextures ; i++)
		if (!strnicmp(textures[i]->name, name, 8))
	    	return i;
		
    return -1;
	unguard;
}

//==========================================================================
//
// 	R_TextureNumForName
//
// 	Calls R_CheckTextureNumForName, aborts with error message.
//
//==========================================================================

int	R_TextureNumForName(const char* name)
{
	guard(R_TextureNumForName);
    int		i;
	
    i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
		Host_Error("R_TextureNumForName: %s not found", name);
    }
    return i;
	unguard;
}

//==========================================================================
//
//	R_TextureHeight
//
//==========================================================================

float R_TextureHeight(int pic)
{
	guard(R_TextureHeight);
	if (pic & TEXF_FLAT)
	{
		return 64.0;
	}
	return textureheight[pic];
	unguard;
}

//==========================================================================
//
//	FlatFunc
//
//==========================================================================

static bool FlatFunc(int lump, const char*, int, EWadNamespace NS)
{
	guard(FlatFunc);
	if (NS == WADNS_Flats)
	{
		//	Add flat
		numflats++;
		Z_Resize((void**)&flatlumps, numflats * 4);
		flatlumps[numflats - 1] = lump;
	}
	return true;	//	Continue
	unguard;
}

//==========================================================================
//
//	InitFlats
//
//==========================================================================

static void InitFlats(void)
{
	guard(InitFlats);
	flatlumps = (int*)Z_Malloc(1, PU_STATIC, 0);
    numflats = 0;

	W_ForEachLump(FlatFunc);

    // Create translation table for global animation.
    flattranslation = (int*)Z_Malloc((numflats + 1) * 4, PU_STATIC, 0);
    for (int i = 0; i < numflats; i++)
		flattranslation[i] = i;
	unguard;
}

//==========================================================================
//
//  R_CheckFlatNumForName
//
//==========================================================================

int R_CheckFlatNumForName(const char* name)
{
	guard(R_CheckFlatNumForName);
	for (int i = numflats - 1; i >= 0; i--)
	{
		if (!strnicmp(name, W_LumpName(flatlumps[i]), 8))
		{
			return i | TEXF_FLAT;
		}
	}
	return -1;
	unguard;
}

//==========================================================================
//
//	R_FlatNumForName
//
//	Retrieval, get a flat number for a flat name.
//
//==========================================================================

int R_FlatNumForName(const char* name)
{
	guard(R_FlatNumForName);
    char	namet[9];
    int		i;

	i = R_CheckFlatNumForName(name);
    if (i == -1)
    {
		namet[8] = 0;
		memcpy(namet, name,8);
		Host_Error("R_FlatNumForName: %s not found",namet);
    }
    return i;
	unguard;
}

//==========================================================================
//
//	SpriteCallback
//
//==========================================================================

static bool SpriteCallback(int lump, const char*, int, EWadNamespace NS)
{
	guard(SpriteCallback);
	if (NS == WADNS_Sprites)
	{
		//	Add sprite lump
		numspritelumps++;
		Z_Resize((void**)&spritelumps, numspritelumps * 4);
		spritelumps[numspritelumps - 1] = lump;
	}
	return true;	//	Continue
	unguard;
}

//==========================================================================
//
//	InitSpriteLumps
//
//==========================================================================

static void InitSpriteLumps(void)
{
	guard(InitSpriteLumps);
    int			i;

	spritelumps = (int*)Z_Malloc (1, PU_STATIC, 0);
    numspritelumps = 0;

	W_ForEachLump(SpriteCallback);

    spritewidth = (int*)Z_Malloc(numspritelumps * 4, PU_STATIC, 0);
    spriteheight = (int*)Z_Malloc(numspritelumps * 4, PU_STATIC, 0);
    spriteoffset = (int*)Z_Malloc(numspritelumps * 4, PU_STATIC, 0);
    spritetopoffset = (int*)Z_Malloc(numspritelumps * 4, PU_STATIC, 0);
	
    for (i = 0; i < numspritelumps; i++)
	{
		spritewidth[i] = -1;
		spriteheight[i] = -1;
		spriteoffset[i] = -1;
		spritetopoffset[i] = -1;
	}
	unguard;
}

//==========================================================================
//
// 	InitTranslationTables
//
//==========================================================================

static void InitTranslationTables(void)
{
	guard(InitTranslationTables);
	translationtables = (byte*)W_CacheLumpName("TRANSLAT", PU_STATIC);
	unguard;
}

//==========================================================================
//
//	InitFTAnims
//
//	Initialize flat and texture animation lists.
//
//==========================================================================

static void InitFTAnims(void)
{
	guard(InitFTAnims);
	int 		base;
	int 		mod;
	animDef_t 	ad;
	frameDef_t	fd;
	bool 		ignore;
	bool	 	done;

	SC_Open(ANIM_SCRIPT_NAME);
	while (SC_GetString())
	{
		memset(&ad, 0, sizeof(ad));
		if (SC_Compare(SCI_FLAT))
		{
			ad.type = ANIM_FLAT;
		}
		else if (SC_Compare(SCI_TEXTURE))
		{
			ad.type = ANIM_TEXTURE;
		}
		else
		{
			SC_ScriptError(NULL);
		}
		SC_MustGetString(); // Name
		ignore = false;
		if (ad.type == ANIM_FLAT)
		{
			if (R_CheckFlatNumForName(sc_String) == -1)
			{
				ignore = true;
			}
			else
			{
				ad.index = R_FlatNumForName(sc_String) & ~TEXF_FLAT;
			}
		}
		else
		{ // Texture
			if (R_CheckTextureNumForName(sc_String) == -1)
			{
				ignore = true;
			}
			else
			{
				ad.index = R_TextureNumForName(sc_String);
			}
		}
		ad.startFrameDef = FrameDefs.Num();
		done = false;
		while (done == false)
		{
			if (SC_GetString())
			{
				if (SC_Compare(SCI_PIC))
				{
					SC_MustGetNumber();
					fd.index = ad.index + sc_Number - 1;
					SC_MustGetString();
					if (SC_Compare(SCI_TICS))
					{
						SC_MustGetNumber();
						fd.baseTime = sc_Number;
						fd.randomRange = 0;
						if (ignore == false)
						{
							FrameDefs.AddItem(fd);
						}
					}
					else if (SC_Compare(SCI_RAND))
					{
						SC_MustGetNumber();
						base = sc_Number;
						SC_MustGetNumber();
						fd.baseTime = base;
						mod = sc_Number - base + 1;
						fd.randomRange = mod;
						if (ignore == false)
						{
							FrameDefs.AddItem(fd);
						}
					}
					else
					{
						SC_ScriptError(NULL);
					}
				}
				else
				{
					SC_UnGet();
					done = true;
				}
			}
			else
			{
				done = true;
			}
		}
		if ((ignore == false) && (FrameDefs.Num() - ad.startFrameDef < 2))
		{
			Sys_Error("P_InitFTAnims: AnimDef has framecount < 2.");
		}
		if (ignore == false)
		{
			ad.endFrameDef = FrameDefs.Num() - 1;
			ad.currentFrameDef = ad.endFrameDef;
			ad.time = 0.01; // Force 1st game tic to animate
			AnimDefs.AddItem(ad);
		}
	}
	SC_Close();
	FrameDefs.Shrink();
	AnimDefs.Shrink();
	unguard;
}

//==========================================================================
//
//	R_AnimateSurfaces
//
//==========================================================================

#ifdef CLIENT
void R_AnimateSurfaces(void)
{
	guard(R_AnimateSurfaces);
	//	Animate flats and textures
	for (TArray<animDef_t>::TIterator ad(AnimDefs); ad; ++ad)
	{
		ad->time -= host_frametime;
		if (ad->time <= 0.0)
		{
			if (ad->currentFrameDef == ad->endFrameDef)
			{
				ad->currentFrameDef = ad->startFrameDef;
			}
			else
			{
				ad->currentFrameDef++;
			}
			frameDef_t fd = FrameDefs[ad->currentFrameDef];
			ad->time = fd.baseTime / 35.0;
			if (fd.randomRange)
			{ 
				// Random tics
				ad->time += Random() * (fd.randomRange / 35.0);
			}
			if (ad->type == ANIM_FLAT)
			{
				flattranslation[ad->index] = fd.index;
			}
			else
			{ 
				// Texture
				texturetranslation[ad->index] = fd.index;
			}
		}
	}

	R_AnimateSky();
	unguard;
}

//==========================================================================
//
//	R_TextureAnimation
//
//==========================================================================

int R_TextureAnimation(int InTex)
{
	guard(R_TextureAnimation);
	int tex = InTex;
	if (tex & TEXF_SKY_MAP)
		return tex;
	if (tex & TEXF_FLAT)
		return TEXF_FLAT | flattranslation[tex & ~TEXF_FLAT];
	else
		return texturetranslation[tex];
	unguard;
}
#endif

//==========================================================================
//
//	R_InitTexture
//
//==========================================================================

void R_InitTexture(void)
{
	guard(R_InitTexture);
	if (IsStrifeTexture())
	{
		GCon->Log(NAME_Init, "Strife textures detected");
		InitTextures2();
	}
	else
	{
		InitTextures();
	}
	InitFlats();
	InitSpriteLumps();
	InitTranslationTables();
	InitFTAnims(); // Init flat and texture animations

    skyflatnum = R_CheckFlatNumForName("F_SKY");
	if (skyflatnum < 0)
	    skyflatnum = R_CheckFlatNumForName("F_SKY001");
	if (skyflatnum < 0)
	    skyflatnum = R_FlatNumForName("F_SKY1");
	unguard;
}

#ifdef CLIENT

//==========================================================================
//
//	R_SetupPalette
//
//==========================================================================

void R_SetupPalette(int palnum, const char *name)
{
	guard(R_SetupPalette);
	byte *psrc = (byte*)W_CacheLumpName(name, PU_TEMP);
	rgba_t *pal = r_palette[palnum];
	//	We use color 0 as transparent color, so we must find an alternate
	// index for black color. In Doom, Heretic and Strife there is another
	// black color, in Hexen it's almost black.
	//	I think that originaly Doom uses color 255 as transparent color, but
	// utilites created by others uses the alternate black color and these
	// graphics can contain pixels of color 255.
	//	Heretic and Hexen also uses color 255 as transparent, even more - in
	// colormaps it's maped to color 0. Posibly this can cause problems with
	// modified graphics.
	//	Strife uses color 0 as transparent. I already had problems with fact
	// that color 255 is normal color, now there shouldn't be any problems.
	int best_dist = 0x10000;
	for (int i = 0; i < 256; i++)
	{
		pal[i].r = *psrc++;
		pal[i].g = *psrc++;
		pal[i].b = *psrc++;
		if (i == 0)
		{
			pal[i].a = 0;
		}
		else
		{
			pal[i].a = 255;
			int dist = pal[i].r * pal[i].r + pal[i].g * pal[i].g +
				pal[i].b * pal[i].b;
			if (dist < best_dist)
			{
				r_black_color[palnum] = i;
				best_dist = dist;
			}
		}
	}
	unguard;
}

//==========================================================================
//
//	R_InitData
//
//==========================================================================

void R_InitData(void)
{
	guard(R_InitData);
	R_SetupPalette(0, "PLAYPAL");
	unguard;
}

//==========================================================================
//
// 	R_PrecacheLevel
//
// 	Preloads all relevant graphics for the level.
//
//==========================================================================

void R_PrecacheLevel(void)
{
	guard(R_PrecacheLevel);
	int			i;
    
	if (cls.demoplayback)
		return;
    
#ifdef __GNUC__
	char flatpresent[numflats];
	char texturepresent[numtextures];
#else
	char* flatpresent = (char*)Z_StrMalloc(numflats);
	char* texturepresent = (char*)Z_StrMalloc(numtextures);
#endif
	memset(flatpresent, 0, numflats);
	memset(texturepresent, 0, numtextures);

#define MARK(tex) \
if (tex & TEXF_FLAT)\
{\
	flatpresent[tex & ~TEXF_FLAT] = true;\
}\
else\
{\
	texturepresent[tex] = true;\
}
	for (i = 0; i < GClLevel->NumSectors; i++)
	{
		MARK(GClLevel->Sectors[i].floor.pic)
		MARK(GClLevel->Sectors[i].ceiling.pic)
	}
	
	for (i = 0; i < GClLevel->NumSides; i++)
	{
		MARK(GClLevel->Sides[i].toptexture)
		MARK(GClLevel->Sides[i].midtexture)
		MARK(GClLevel->Sides[i].bottomtexture)
	}

	// Precache flats.
	for (i = 0; i < numflats; i++)
	{
		if (flatpresent[i])
		{
			Drawer->SetFlat(i);
		}
	}

	// Precache textures.
	for (i = 0; i < numtextures; i++)
	{
		if (texturepresent[i])
		{
			Drawer->SetTexture(i);
		}
	}

#ifndef __GNUC__
	Z_Free(flatpresent);
	Z_Free(texturepresent);
#endif
	unguard;
}

//==========================================================================
//
//	R_RegisterPic
//
//==========================================================================

int R_RegisterPic(const char *name, int type)
{
	guard(R_RegisterPic);
	char tmpName[MAX_VPATH];
	if (type == PIC_PATCH || type == PIC_RAW)
	{
		W_CleanupName(name, tmpName);
		if (W_CheckNumForName(tmpName) < 0 &&
			W_CheckNumForName(tmpName, WADNS_Sprites) < 0)
		{
			GCon->Logf("R_RegisterPic: Pic %s not found", tmpName);
			return -1;
		}
	}
	else
	{
		strcpy(tmpName, name);
	}
	for (int i = 0; i < MAX_PICS; i++)
	{
		if (!pic_list[i].name[0])
		{
			strcpy(pic_list[i].name, tmpName);
			pic_list[i].type = type;
			pic_list[i].palnum = 0;
			return i;
		}
		if (!stricmp(pic_list[i].name, tmpName))
		{
			return i;
		}
	}
	GCon->Log(NAME_Dev, "R_RegisterPic: No more free slots");
	return -1;
	unguard;
}

//==========================================================================
//
//	R_RegisterPicPal
//
//==========================================================================

int R_RegisterPicPal(const char *name, int type, const char *palname)
{
	guard(R_RegisterPicPal);
	char tmpName[MAX_VPATH];
	if (type == PIC_PATCH || type == PIC_RAW)
	{
		W_CleanupName(name, tmpName);
		if (W_CheckNumForName(tmpName) < 0 &&
			W_CheckNumForName(tmpName, WADNS_Sprites) < 0)
		{
			GCon->Logf("R_RegisterPic: Pic %s not found", tmpName);
			return -1;
		}
	}
	else
	{
		strcpy(tmpName, name);
	}
	for (int i = 0; i < MAX_PICS; i++)
	{
		if (!pic_list[i].name[0])
		{
			strcpy(pic_list[i].name, tmpName);
			pic_list[i].type = type;
			pic_list[i].palnum = 1;
			R_SetupPalette(1, palname);
			return i;
		}
		if (!stricmp(pic_list[i].name, tmpName))
		{
			return i;
		}
	}
	GCon->Log(NAME_Dev, "R_RegisterPic: No more free slots");
	return -1;
	unguard;
}

//==========================================================================
//
//	R_GetPicInfo
//
//==========================================================================

void R_GetPicInfo(int handle, picinfo_t *info)
{
	guard(R_GetPicInfo);
	if (handle < 0)
	{
		memset(info, 0, sizeof(*info));
	}
	else
	{
		if (!pic_list[handle].width)
		{
			int LumpNum;
			patch_t *patch;

			switch (pic_list[handle].type)
			{
			case PIC_PATCH:
				LumpNum = W_CheckNumForName(pic_list[handle].name);
				//	Some inventory pics are inside sprites.
				if (LumpNum < 0)
					LumpNum = W_GetNumForName(pic_list[handle].name, 
						WADNS_Sprites);
				patch = (patch_t*)W_CacheLumpNum(LumpNum, PU_CACHE);
				pic_list[handle].width = LittleShort(patch->width);
				pic_list[handle].height = LittleShort(patch->height);
				pic_list[handle].xoffset = LittleShort(patch->leftoffset);
				pic_list[handle].yoffset = LittleShort(patch->topoffset);
				break;

			case PIC_RAW:
				pic_list[handle].width = 320;
				pic_list[handle].height = 200;
				pic_list[handle].xoffset = 0;
				pic_list[handle].yoffset = 0;
				break;
			}
		}
		info->width = pic_list[handle].width;
		info->height = pic_list[handle].height;
		info->xoffset = pic_list[handle].xoffset;
		info->yoffset = pic_list[handle].yoffset;
	}
	unguard;
}

//==========================================================================
//
//	R_DrawPic
//
//==========================================================================

void R_DrawPic(int x, int y, int handle, int trans)
{
	guard(R_DrawPic);
	picinfo_t	info;

	if (handle < 0)
	{
		return;
	}

	R_GetPicInfo(handle, &info);
	x -= info.xoffset;
	y -= info.yoffset;
	Drawer->DrawPic(fScaleX * x, fScaleY * y,
		fScaleX * (x + info.width), fScaleY * (y + info.height),
		0, 0, info.width, info.height, handle, trans);
	unguard;
}

//==========================================================================
//
//	R_DrawShadowedPic
//
//==========================================================================

void R_DrawShadowedPic(int x, int y, int handle)
{
	guard(R_DrawShadowedPic);
	picinfo_t	info;

	if (handle < 0)
	{
		return;
	}

	R_GetPicInfo(handle, &info);
	x -= info.xoffset;
	y -= info.yoffset;
	Drawer->DrawPicShadow(fScaleX * (x + 2), fScaleY * (y + 2),
		fScaleX * (x + 2 + info.width), fScaleY * (y + 2 + info.height),
		0, 0, info.width, info.height, handle, 160);
	Drawer->DrawPic(fScaleX * x, fScaleY * y,
		fScaleX * (x + info.width), fScaleY * (y + info.height),
		0, 0, info.width, info.height, handle, 0);
	unguard;
}

//==========================================================================
//
//  R_FillRectWithFlat
//
// 	Fills rectangle with flat.
//
//==========================================================================

void R_FillRectWithFlat(int DestX, int DestY, int width, int height, const char* fname)
{
	guard(R_FillRectWithFlat);
	Drawer->FillRectWithFlat(fScaleX * DestX, fScaleY * DestY,
		fScaleX * (DestX + width), fScaleY * (DestY + height),
		0, 0, width, height, fname);
	unguard;
}

//==========================================================================
//
//	V_DarkenScreen
//
//  Fade all the screen buffer, so that the menu is more readable,
// especially now that we use the small hufont in the menus...
//
//==========================================================================

void V_DarkenScreen(int darkening)
{
	guard(V_DarkenScreen);
	Drawer->ShadeRect(0, 0, ScreenWidth, ScreenHeight, darkening);
	unguard;
}

//==========================================================================
//
//	R_ShadeRect
//
//==========================================================================

void R_ShadeRect(int x, int y, int width, int height, int shade)
{
	guard(R_ShadeRect);
	Drawer->ShadeRect((int)(x * fScaleX), (int)(y * fScaleY),
		(int)((x + width) * fScaleX) - (int)(x * fScaleX),
		(int)((y + height) * fScaleY) - (int)(y * fScaleY), shade);
	unguard;
}

#endif

//**************************************************************************
//
//	$Log$
//	Revision 1.25  2005/04/28 07:16:15  dj_jl
//	Fixed some warnings, other minor fixes.
//
//	Revision 1.24  2004/12/22 07:51:52  dj_jl
//	Warning about non-existing graphics.
//	
//	Revision 1.23  2004/11/23 12:43:10  dj_jl
//	Wad file lump namespaces.
//	
//	Revision 1.22  2004/08/18 18:05:47  dj_jl
//	Support for higher virtual screen resolutions.
//	
//	Revision 1.21  2002/09/07 16:31:51  dj_jl
//	Added Level class.
//	
//	Revision 1.20  2002/07/27 18:10:11  dj_jl
//	Implementing Strife conversations.
//	
//	Revision 1.19  2002/07/13 07:51:49  dj_jl
//	Replacing console's iostream with output device.
//	
//	Revision 1.18  2002/03/28 17:58:02  dj_jl
//	Added support for scaled textures.
//	
//	Revision 1.17  2002/03/20 19:11:21  dj_jl
//	Added guarding.
//	
//	Revision 1.16  2002/02/22 18:09:52  dj_jl
//	Some improvements, beautification.
//	
//	Revision 1.15  2002/01/07 12:16:43  dj_jl
//	Changed copyright year
//	
//	Revision 1.14  2001/12/27 17:36:47  dj_jl
//	Some speedup
//	
//	Revision 1.13  2001/12/12 19:26:40  dj_jl
//	Added dynamic arrays
//	
//	Revision 1.12  2001/11/09 14:22:10  dj_jl
//	R_InitTexture now called from Host_init
//	
//	Revision 1.11  2001/10/18 17:36:31  dj_jl
//	A lots of changes for Alpha 2
//	
//	Revision 1.10  2001/10/08 17:34:57  dj_jl
//	A lots of small changes and cleanups
//	
//	Revision 1.9  2001/10/04 17:23:29  dj_jl
//	Got rid of some warnings
//	
//	Revision 1.8  2001/08/30 17:44:07  dj_jl
//	Removed memory leaks after startup
//	
//	Revision 1.7  2001/08/23 17:47:22  dj_jl
//	Started work on pics with custom palettes
//	
//	Revision 1.6  2001/08/21 17:46:08  dj_jl
//	Added R_TextureAnimation, made SetTexture recognize flats
//	
//	Revision 1.5  2001/08/15 17:21:14  dj_jl
//	Removed game dependency
//	
//	Revision 1.4  2001/08/01 17:33:58  dj_jl
//	Fixed drawing of spite lump for player setup menu, beautification
//	
//	Revision 1.3  2001/07/31 17:16:31  dj_jl
//	Just moved Log to the end of file
//	
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
