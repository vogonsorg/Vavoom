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
//**
//**	Functions to draw patches (by post) directly to screen.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gl_local.h"

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
//	TOpenGLDrawer::DrawPic
//
//==========================================================================

void TOpenGLDrawer::DrawPic(float x1, float x2, float y1, float y2,
	float s1, float s2, float t1, float t2, int handle, int trans)
{
	SetPic(handle);
	if (trans)
	{
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, (100.0 - trans) / 100.0);
	}
	else
	{
		glColor4f(1, 1, 1, 1);
	}
	glBegin(GL_QUADS);
	glTexCoord2f(s1 * tex_iw, t1 * tex_ih);
	glVertex2f(x1, y1);
	glTexCoord2f(s2 * tex_iw, t1 * tex_ih);
	glVertex2f(x2, y1);
	glTexCoord2f(s2 * tex_iw, t2 * tex_ih);
	glVertex2f(x2, y2);
	glTexCoord2f(s1 * tex_iw, t2 * tex_ih);
	glVertex2f(x1, y2);
	glEnd();
	if (trans)
	{
		glDisable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
	}
}

//==========================================================================
//
//	TOpenGLDrawer::DrawPicShadow
//
//==========================================================================

void TOpenGLDrawer::DrawPicShadow(float x1, float x2, float y1, float y2,
	float s1, float s2, float t1, float t2, int handle, int shade)
{
	SetPic(handle);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glColor4f(0, 0, 0, (float)shade / 255.0);
	glBegin(GL_QUADS);
	glTexCoord2f(s1 * tex_iw, t1 * tex_ih);
	glVertex2f(x1, y1);
	glTexCoord2f(s2 * tex_iw, t1 * tex_ih);
	glVertex2f(x2, y1);
	glTexCoord2f(s2 * tex_iw, t2 * tex_ih);
	glVertex2f(x2, y2);
	glTexCoord2f(s1 * tex_iw, t2 * tex_ih);
	glVertex2f(x1, y2);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
}

//==========================================================================
//
//  TOpenGLDrawer::FillRectWithFlat
//
// 	Fills rectangle with flat.
//
//==========================================================================

void TOpenGLDrawer::FillRectWithFlat(int x, int y, int width, int height, const char* fname)
{
	SetFlat(R_FlatNumForName(fname));

	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);
	glTexCoord2f(width / 64.0, 0);
	glVertex2f(x + width, y);
	glTexCoord2f(width / 64.0, height / 64.0);
	glVertex2f(x + width, y + height);
	glTexCoord2f(0, height / 64.0);
	glVertex2f(x, y + height);
	glEnd();
}

//==========================================================================
//
//	TOpenGLDrawer::ShadeRect
//
//  Fade all the screen buffer, so that the menu is more readable,
// especially now that we use the small hufont in the menus...
//
//==========================================================================

void TOpenGLDrawer::ShadeRect(int x, int y, int w, int h, int darkening)
{
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(0, 0, 0, darkening / 32.0);
	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

//==========================================================================
//
//	TOpenGLDrawer::DrawConsoleBackground
//
//==========================================================================

void TOpenGLDrawer::DrawConsoleBackground(int h)
{
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(0, 0, 0.5, 0.75);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(ScreenWidth, 0);
	glVertex2f(ScreenWidth, h);
	glVertex2f(0, h);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

//==========================================================================
//
//	TOpenGLDrawer::DrawSpriteLump
//
//==========================================================================

void TOpenGLDrawer::DrawSpriteLump(int x, int y, int lump, int translation, boolean flip)
{
	SetSpriteLump(lump, translation);

    x -= (int)spriteoffset[lump];
   	y -= (int)spritetopoffset[lump];

	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
    if (flip)
	{
		glTexCoord2f(1, 0);
		glVertex2f(x, y);
		glTexCoord2f(0, 0);
		glVertex2f(x + 256, y);
		glTexCoord2f(0, 1);
		glVertex2f(x + 256, y + 256);
		glTexCoord2f(1, 1);
		glVertex2f(x, y + 256);
	}
    else
	{
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glTexCoord2f(1, 0);
		glVertex2f(x + 256, y);
		glTexCoord2f(1, 1);
		glVertex2f(x + 256, y + 256);
		glTexCoord2f(0, 1);
		glVertex2f(x, y + 256);
	}
	glEnd();
}


//==========================================================================
//
//	TOpenGLDrawer::StartAutomap
//
//==========================================================================

void TOpenGLDrawer::StartAutomap(void)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBegin(GL_LINES);
}

//==========================================================================
//
//	TOpenGLDrawer::DrawLine
//
//==========================================================================

void TOpenGLDrawer::DrawLine(int x1, int y1, dword c1, int x2, int y2, dword c2)
{
	glColor4ub(pal8_to24[c1].r, pal8_to24[c1].g, pal8_to24[c1].b, 255);
	glVertex2f(x1, y1);
	glColor4ub(pal8_to24[c2].r, pal8_to24[c2].g, pal8_to24[c2].b, 255);
	glVertex2f(x2, y2);
}

//==========================================================================
//
//	TOpenGLDrawer::EndAutomap
//
//==========================================================================

void TOpenGLDrawer::EndAutomap(void)
{
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

