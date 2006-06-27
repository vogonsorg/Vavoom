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
//**	Copyright (C) 1999-2006 Jānis Legzdiņš
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
//
//	Dynamic string class.
//
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	VStr::VStr
//
//==========================================================================

VStr::VStr(const VStr& InStr, int Start, int Len)
: Str(NULL)
{
	check(Start >= 0);
	check(Start <= (int)InStr.Length());
	check(Len >= 0);
	check(Start + Len <= (int)InStr.Length());
	if (Len)
	{
		Resize(Len);
		strncpy(Str, InStr.Str + Start, Len);
	}
}

//==========================================================================
//
//	VStr::Resize
//
//==========================================================================

void VStr::Resize(int NewLen)
{
	guard(VStr::Resize);
	check(NewLen >= 0);
	if (!NewLen)
	{
		//	Free string.
		if (Str)
		{
			delete[] (Str - sizeof(int));
			Str = NULL;
		}
	}
	else
	{
		//	Allocate memory.
		int AllocLen = sizeof(int) + NewLen + 1;
		char* NewStr = (new char[AllocLen]) + sizeof(int);
		if (Str)
		{
			size_t Len = Min(Length(), (size_t)NewLen);
			strncpy(NewStr, Str, Len);
			delete[] (Str - sizeof(int));
		}
		Str = NewStr;
		//	Set length.
		((int*)Str)[-1] = NewLen;
		//	Set terminator.
		Str[NewLen] = 0;
	}
	unguard;
}

//==========================================================================
//
//	VStr::EvalEscapeSequences
//
//==========================================================================

VStr VStr::EvalEscapeSequences() const
{
	guard(VStr::EvalEscapeSequences);
	VStr Ret;
	char Val;
	for (const char* c = **this; *c; c++)
	{
		if (*c == '\\')
		{
			c++;
			switch (*c)
			{
			case 't':
				Ret += '\t';
				break;
			case 'n':
				Ret += '\n';
				break;
			case 'r':
				Ret += '\r';
				break;
			case 'x':
				Val = 0;
				c++;
				for (int i = 0; i < 2; i++)
				{
					if (*c >= '0' && *c <= '9')
						Val = (Val << 4) + *c - '0';
					else if (*c >= 'a' && *c <= 'f')
						Val = (Val << 4) + 10 + *c - 'a';
					else if (*c >= 'A' && *c <= 'F')
						Val = (Val << 4) + 10 + *c - 'A';
					else
						break;
					c++;
				}
				c--;
				Ret += Val;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				Val = 0;
				for (int i = 0; i < 3; i++)
				{
					if (*c >= '0' && *c <= '7')
						Val = (Val << 3) + *c - '0';
					else
						break;
					c++;
				}
				c--;
				Ret += Val;
				break;
			case '\n':
				break;
			case 0:
				c--;
				break;
			default:
				Ret += *c;
				break;
			}
		}
		else
		{
			Ret += *c;
		}
	}
	return Ret;
	unguard;
}

//==========================================================================
//
//	VStr::ExtractFilePath
//
//==========================================================================

VStr VStr::ExtractFilePath() const
{
	guard(FL_ExtractFilePath);
	const char* src = Str + Length() - 1;

	//
	// back up until a \ or the start
	//
	while (src != Str && *(src - 1) != '/' && *(src - 1) != '\\')
		src--;

	return VStr(*this, 0, src - Str);
	unguard;
}

//==========================================================================
//
//	VStr:ExtractFileName
//
//==========================================================================

VStr VStr::ExtractFileName() const
{
	guard(VStr:ExtractFileName);
	const char* src = Str + Length() - 1;

	//
	// back up until a \ or the start
	//
	while (src != Str && *(src - 1) != '/' && *(src - 1) != '\\')
		src--;

	return src;
	unguard;
}

//==========================================================================
//
//	VStr::ExtractFileBase
//
//==========================================================================

VStr VStr::ExtractFileBase() const
{
	guard(VStr::ExtractFileBase);
	int i = Length() - 1;

	// back up until a \ or the start
	while (i && Str[i - 1] != '\\' && Str[i - 1] != '/')
	{
		i--;
	}

	// copy up to eight characters
	int start = i;
	int length = 0;
	while (Str[i] && Str[i] != '.')
	{
		if (++length == 9)
			Sys_Error("Filename base of %s >8 chars", Str);
		i++;
	}
	return VStr(*this, start, length);
	unguard;
}

//==========================================================================
//
//	VStr::ExtractFileExtension
//
//==========================================================================

VStr VStr::ExtractFileExtension() const
{
	guard(VStr::ExtractFileExtension);
	const char* src = Str + Length() - 1;

	//
	// back up until a . or the start
	//
	while (src != Str && *(src - 1) != '.')
		src--;
	if (src == Str)
	{
		return VStr();	// no extension
	}

	return src;
	unguard;
}

//==========================================================================
//
//	VStr::StripExtension
//
//==========================================================================

VStr VStr::StripExtension() const
{
	guard(VStr::StripExtension);
	const char* search = Str + Length() - 1;
	while (*search != '/' && *search != '\\' && search != Str)
	{
		if (*search == '.')
		{
			return VStr(*this, 0, search - Str);
		}
		search--;
	}
	return *this;
	unguard;
}

//==========================================================================
//
//	VStr::DefaultPath
//
//==========================================================================

VStr VStr::DefaultPath(const VStr& basepath) const
{
	guard(VStr::DefaultPath);
	if (Str[0] == '/')
	{
		return *this;	// absolute path location
	}
	return basepath + *this;
	unguard;
}

//==========================================================================
//
//	VStr.DefaultExtension
//
//==========================================================================

VStr VStr::DefaultExtension(const VStr& extension) const
{
	guard(VStr::DefaultExtension);
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	const char* src = Str + Length() - 1;

	while (*src != '/' && *src != '\\' && src != Str)
	{
		if (*src == '.')
        {
			return *this;	// it has an extension
		}
		src--;
	}

	return *this + extension;
	unguard;
}
