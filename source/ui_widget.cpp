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

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"
#include "cl_local.h"
#include "ui.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

IMPLEMENT_CLASS(V, Widget);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	VWidget::AddChild
//
//==========================================================================

void VWidget::AddChild(VWidget* NewChild)
{
	guard(VWidget::AddChild);
	NewChild->PrevWidget = LastChildWidget;
	NewChild->NextWidget = NULL;
	if (LastChildWidget)
	{
		LastChildWidget->NextWidget = NewChild;
	}
	else
	{
		FirstChildWidget = NewChild;
	}
	LastChildWidget = NewChild;
	OnChildAdded(NewChild);
	unguard;
}

//==========================================================================
//
//	VWidget::RemoveChild
//
//==========================================================================

void VWidget::RemoveChild(VWidget* InChild)
{
	guard(VWidget::RemoveChild);
	if (InChild->PrevWidget)
	{
		InChild->PrevWidget->NextWidget = InChild->NextWidget;
	}
	else
	{
		FirstChildWidget = InChild->NextWidget;
	}
	if (InChild->NextWidget)
	{
		InChild->NextWidget->PrevWidget = InChild->PrevWidget;
	}
	else
	{
		LastChildWidget = InChild->PrevWidget;
	}
	InChild->PrevWidget = NULL;
	InChild->NextWidget = NULL;
	InChild->ParentWidget = NULL;
	OnChildRemoved(InChild);
	unguard;
}

//==========================================================================
//
//	VWidget::Lower
//
//==========================================================================

void VWidget::Lower()
{
	guard(VWidget::Lower);
	if (!ParentWidget)
	{
		Sys_Error("Can't lower root window");
	}

	if (ParentWidget->FirstChildWidget == this)
	{
		//	Already there
		return;
	}

	//	Unlink from current location
	PrevWidget->NextWidget = NextWidget;
	if (NextWidget)
	{
		NextWidget->PrevWidget = PrevWidget;
	}
	else
	{
		ParentWidget->LastChildWidget = PrevWidget;
	}

	//	Link on bottom
	PrevWidget = NULL;
	NextWidget = ParentWidget->FirstChildWidget;
	ParentWidget->FirstChildWidget->PrevWidget = this;
	ParentWidget->FirstChildWidget = this;
	unguard;
}

//==========================================================================
//
//	VWidget::Raise
//
//==========================================================================

void VWidget::Raise()
{
	guard(VWidget::Raise);
	if (!ParentWidget)
	{
		Sys_Error("Can't raise root window");
	}

	if (ParentWidget->LastChildWidget == this)
	{
		//	Already there
		return;
	}

	//	Unlink from current location
	NextWidget->PrevWidget = PrevWidget;
	if (PrevWidget)
	{
		PrevWidget->NextWidget = NextWidget;
	}
	else
	{
		ParentWidget->FirstChildWidget = NextWidget;
	}

	//	Link on top
	PrevWidget = ParentWidget->LastChildWidget;
	NextWidget = NULL;
	ParentWidget->LastChildWidget->NextWidget = this;
	ParentWidget->LastChildWidget = this;
	unguard;
}

//==========================================================================
//
//	VWidget::ClipTree
//
//==========================================================================

void VWidget::ClipTree()
{
	guard(VWidget::ClipTree);
	//	Set up clipping rectangle.
	if (ParentWidget)
	{
		//	Clipping rectangle is relative to the parent widget.
		ClipRect.OriginX = ParentWidget->ClipRect.OriginX +
			ParentWidget->ClipRect.ScaleX * PosX;
		ClipRect.OriginY = ParentWidget->ClipRect.OriginY +
			ParentWidget->ClipRect.ScaleY * PosY;
		ClipRect.ScaleX = ParentWidget->ClipRect.ScaleX * SizeScaleX;
		ClipRect.ScaleY = ParentWidget->ClipRect.ScaleY * SizeScaleY;
		ClipRect.ClipX1 = ClipRect.OriginX;
		ClipRect.ClipY1 = ClipRect.OriginY;
		ClipRect.ClipX2 = ClipRect.OriginX + ClipRect.ScaleX * SizeWidth;
		ClipRect.ClipY2 = ClipRect.OriginY + ClipRect.ScaleY * SizeHeight;

		//	Clip against the parent widget's clipping rectangle.
		if (ClipRect.ClipX1 < ParentWidget->ClipRect.ClipX1)
		{
			ClipRect.ClipX1 = ParentWidget->ClipRect.ClipX1;
		}
		if (ClipRect.ClipY1 < ParentWidget->ClipRect.ClipY1)
		{
			ClipRect.ClipY1 = ParentWidget->ClipRect.ClipY1;
		}
		if (ClipRect.ClipX2 > ParentWidget->ClipRect.ClipX2)
		{
			ClipRect.ClipX2 = ParentWidget->ClipRect.ClipX2;
		}
		if (ClipRect.ClipY2 > ParentWidget->ClipRect.ClipY2)
		{
			ClipRect.ClipY2 = ParentWidget->ClipRect.ClipY2;
		}
	}
	else
	{
		//	This is the root widget.
		ClipRect.OriginX = PosX;
		ClipRect.OriginY = PosY;
		ClipRect.ScaleX = SizeScaleX;
		ClipRect.ScaleY = SizeScaleY;
		ClipRect.ClipX1 = ClipRect.OriginX;
		ClipRect.ClipY1 = ClipRect.OriginY;
		ClipRect.ClipX2 = ClipRect.OriginX + ClipRect.ScaleX * SizeWidth;
		ClipRect.ClipY2 = ClipRect.OriginY + ClipRect.ScaleY * SizeHeight;
	}

	//	Set up clipping rectangles in child widgets.
	for (VWidget* W = FirstChildWidget; W; W = W->NextWidget)
	{
		W->ClipTree();
	}
	unguard;
}

//==========================================================================
//
//	VWidget::SetConfiguration
//
//==========================================================================

void VWidget::SetConfiguration(int NewX, int NewY, int NewWidth,
	int HewHeight, float NewScaleX, float NewScaleY)
{
	guard(VWidget::SetConfiguration);
	PosX = NewX;
	PosY = NewY;
	SizeWidth = NewWidth;
	SizeHeight = HewHeight;
	SizeScaleX = NewScaleX;
	SizeScaleY = NewScaleY;
	ClipTree();
	OnConfigurationChanged();
	unguard;
}

//==========================================================================
//
//	VWidget::TransferAndClipRect
//
//==========================================================================

void VWidget::TransferAndClipRect(float& X1, float& Y1, float& X2, float& Y2,
	float& S1, float& T1, float& S2, float& T2) const
{
	guard(VWidget::TransferAndClipRect);
	X1 = ClipRect.ScaleX * X1 + ClipRect.OriginX;
	Y1 = ClipRect.ScaleY * Y1 + ClipRect.OriginY;
	X2 = ClipRect.ScaleX * X2 + ClipRect.OriginX;
	Y2 = ClipRect.ScaleY * Y2 + ClipRect.OriginY;
	if (X1 < ClipRect.ClipX1)
	{
		S1 = S1 + (X1 - ClipRect.ClipX1) / (X1 - X2) * (S2 - S1);
		X1 = ClipRect.ClipX1;
	}
	if (X2 > ClipRect.ClipX2)
	{
		S2 = S2 + (X2 - ClipRect.ClipX2) / (X1 - X2) * (S2 - S1);
		X2 = ClipRect.ClipX2;
	}
	if (Y1 < ClipRect.ClipY1)
	{
		T1 = T1 + (Y1 - ClipRect.ClipY1) / (Y1 - Y2) * (T2 - T1);
		Y1 = ClipRect.ClipY1;
	}
	if (Y2 > ClipRect.ClipY2)
	{
		T2 = T2 + (Y2 - ClipRect.ClipY2) / (Y1 - Y2) * (T2 - T1);
		Y2 = ClipRect.ClipY2;
	}
	unguard;
}

//==========================================================================
//
//	VWidget::DrawPic
//
//==========================================================================

void VWidget::DrawPic(int X, int Y, int Handle, float Alpha)
{
	guard(VWidget::DrawPic);
	if (Handle < 0)
	{
		return;
	}

	picinfo_t Info;
	GTextureManager.GetTextureInfo(Handle, &Info);
	X -= Info.xoffset;
	Y -= Info.yoffset;
	float X1 = X;
	float Y1 = Y;
	float X2 = X + Info.width;
	float Y2 = Y + Info.height;
	float S1 = 0;
	float T1 = 0;
	float S2 = Info.width;
	float T2 = Info.height;
	TransferAndClipRect(X1, Y1, X2, Y2, S1, T1, S2, T2);
	Drawer->DrawPic(X1, Y1, X2, Y2, S1, T1, S2, T2, Handle, Alpha);
	unguard;
}

//==========================================================================
//
//	VWidget::DrawShadowedPic
//
//==========================================================================

void VWidget::DrawShadowedPic(int X, int Y, int Handle)
{
	guard(VWidget::DrawShadowedPic);
	if (Handle < 0)
	{
		return;
	}

	picinfo_t Info;
	GTextureManager.GetTextureInfo(Handle, &Info);
	float X1 = X - Info.xoffset + 2;
	float Y1 = Y - Info.yoffset + 2;
	float X2 = X - Info.xoffset + 2 + Info.width;
	float Y2 = Y - Info.yoffset + 2 + Info.height;
	float S1 = 0;
	float T1 = 0;
	float S2 = Info.width;
	float T2 = Info.height;
	TransferAndClipRect(X1, Y1, X2, Y2, S1, T1, S2, T2);
	Drawer->DrawPicShadow(X1, Y1, X2, Y2, S1, T1, S2, T2, Handle, 0.625);

	DrawPic(X, Y, Handle);
	unguard;
}

//==========================================================================
//
//	VWidget::FillRectWithFlat
//
//==========================================================================

void VWidget::FillRectWithFlat(int X, int Y, int Width, int Height,
	VName Name)
{
	guard(VWidget::FillRectWithFlat);
	float X1 = X;
	float Y1 = Y;
	float X2 = X + Width;
	float Y2 = Y + Height;
	float S1 = 0;
	float T1 = 0;
	float S2 = Width;
	float T2 = Height;
	TransferAndClipRect(X1, Y1, X2, Y2, S1, T1, S2, T2);
	Drawer->FillRectWithFlat(X1, Y1, X2, Y2, S1, T1, S2, T2, Name);
	unguard;
}

//==========================================================================
//
//	VWidget::ShadeRect
//
//==========================================================================

void VWidget::ShadeRect(int X, int Y, int Width, int Height, float Shade)
{
	guard(VWidget::ShadeRect);
	float X1 = X;
	float Y1 = Y;
	float X2 = X + Width;
	float Y2 = Y + Height;
	float S1 = 0;
	float T1 = 0;
	float S2 = 0;
	float T2 = 0;
	TransferAndClipRect(X1, Y1, X2, Y2, S1, T1, S2, T2);
	Drawer->ShadeRect((int)X1, (int)Y1, (int)X2 - (int)X1, (int)Y2 - (int)Y1, Shade);
	unguard;
}

//==========================================================================
//
//	Natives
//
//==========================================================================

IMPLEMENT_FUNCTION(VWidget, Lower)
{
	P_GET_SELF;
	Self->Lower();
}

IMPLEMENT_FUNCTION(VWidget, Raise)
{
	P_GET_SELF;
	Self->Raise();
}

IMPLEMENT_FUNCTION(VWidget, SetPos)
{
	P_GET_INT(NewY);
	P_GET_INT(NewX);
	P_GET_SELF;
	Self->SetPos(NewX, NewY);
}

IMPLEMENT_FUNCTION(VWidget, SetX)
{
	P_GET_INT(NewX);
	P_GET_SELF;
	Self->SetX(NewX);
}

IMPLEMENT_FUNCTION(VWidget, SetY)
{
	P_GET_INT(NewY);
	P_GET_SELF;
	Self->SetY(NewY);
}

IMPLEMENT_FUNCTION(VWidget, SetSize)
{
	P_GET_INT(NewHeight);
	P_GET_INT(NewWidth);
	P_GET_SELF;
	Self->SetSize(NewWidth, NewHeight);
}

IMPLEMENT_FUNCTION(VWidget, SetWidth)
{
	P_GET_INT(NewWidth);
	P_GET_SELF;
	Self->SetWidth(NewWidth);
}

IMPLEMENT_FUNCTION(VWidget, SetHeight)
{
	P_GET_INT(NewHeight);
	P_GET_SELF;
	Self->SetHeight(NewHeight);
}

IMPLEMENT_FUNCTION(VWidget, SetScale)
{
	P_GET_FLOAT(NewScaleY);
	P_GET_FLOAT(NewScaleX);
	P_GET_SELF;
	Self->SetScale(NewScaleX, NewScaleY);
}

IMPLEMENT_FUNCTION(VWidget, SetConfiguration)
{
	P_GET_FLOAT_OPT(NewScaleY, 1.0);
	P_GET_FLOAT_OPT(NewScaleX, 1.0);
	P_GET_INT(NewHeight);
	P_GET_INT(NewWidth);
	P_GET_INT(NewY);
	P_GET_INT(NewX);
	P_GET_SELF;
	Self->SetConfiguration(NewX, NewY, NewWidth, NewHeight, NewScaleX,
		NewScaleY);
}

IMPLEMENT_FUNCTION(VWidget, DrawPic)
{
	P_GET_FLOAT_OPT(Alpha, 1.0);
	P_GET_INT(Handle);
	P_GET_INT(Y);
	P_GET_INT(X);
	P_GET_SELF;
	Self->DrawPic(X, Y, Handle, Alpha);
}

IMPLEMENT_FUNCTION(VWidget, DrawShadowedPic)
{
	P_GET_INT(Handle);
	P_GET_INT(Y);
	P_GET_INT(X);
	P_GET_SELF;
	Self->DrawShadowedPic(X, Y, Handle);
}

IMPLEMENT_FUNCTION(VWidget, FillRectWithFlat)
{
	P_GET_NAME(Name);
	P_GET_INT(Height);
	P_GET_INT(Width);
	P_GET_INT(Y);
	P_GET_INT(X);
	P_GET_SELF;
	Self->FillRectWithFlat(X, Y, Width, Height, Name);
}

IMPLEMENT_FUNCTION(VWidget, ShadeRect)
{
	P_GET_FLOAT(Shade);
	P_GET_INT(Height);
	P_GET_INT(Width);
	P_GET_INT(Y);
	P_GET_INT(X);
	P_GET_SELF;
	Self->ShadeRect(X, Y, Width, Height, Shade);
}