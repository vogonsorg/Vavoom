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

#include "../../makeinfo.h"

int		shadow = 66;
int		altshadow = 0;

char* ammo_names[] =
{
    "am_clip",
    "am_electric_arrow",
    "am_poison_arrow",
    "am_cell",
    "am_misl",
    "am_grenade1",
    "am_grenade2",
    "NUMAMMO",
    "am_noammo"
};

char* weapon_names[] =
{
    "WeaponFist",
    "WeaponCrossbow",
    "WeaponRiffle",
    "WeaponMissile",
    "WeaponGrenade",
    "WeaponFlamethrower",
    "WeaponBlaster",
    "WeaponSigil",
};

int numweapons = 8;

weaponinfo_t weaponinfo[] =
{
	{
		8, //ammo
		15, //upstate
		14, //downstate
		13, //readystate
		16, //atkstate
		0, //holdatkstate
		1, //flashstate
	},
	{
		1, //ammo
		23, //upstate
		22, //downstate
		21, //readystate
		24, //atkstate
		0, //holdatkstate
		1, //flashstate
	},
	{
		0, //ammo
		56, //upstate
		55, //downstate
		54, //readystate
		57, //atkstate
		0, //holdatkstate
		1, //flashstate
	},
	{
		4, //ammo
		46, //upstate
		45, //downstate
		44, //readystate
		47, //atkstate
		0, //holdatkstate
		0, //flashstate
	},
	{
		5, //ammo
		97, //upstate
		96, //downstate
		95, //readystate
		98, //atkstate
		103, //holdatkstate
		0, //flashstate
	},
	{
		3, //ammo
		65, //upstate
		64, //downstate
		62, //readystate
		66, //atkstate
		0, //holdatkstate
		1, //flashstate
	},
	{
		3, //ammo
		73, //upstate
		72, //downstate
		68, //readystate
		74, //atkstate
		0, //holdatkstate
		0, //flashstate
	},
	{
		8, //ammo
		123, //upstate
		122, //downstate
		117, //readystate
		124, //atkstate
		128, //holdatkstate
		0, //flashstate
	},
};

//**************************************************************************
//
//	$Log$
//	Revision 1.5  2002/04/22 17:19:54  dj_jl
//	Retail Strife data.
//
//	Revision 1.4  2002/01/07 12:31:35  dj_jl
//	Changed copyright year
//	
//	Revision 1.3  2001/09/20 16:35:58  dj_jl
//	Beautification
//	
//	Revision 1.2  2001/07/27 14:27:56  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************