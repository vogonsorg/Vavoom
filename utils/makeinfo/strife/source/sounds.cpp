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

int num_sfx = 136;

sfxinfo_t sfx[] =
{
	{ "", "", 0, 2, 1 },
	{ "Sound1", "dsswish", 64, 2, 1 },
	{ "Sound2", "dsmeatht", 64, 2, 1 },
	{ "Sound3", "dsmtalht", 64, 2, 1 },
	{ "Sound4", "dswpnup", 78, 2, 1 },
	{ "Sound5", "dsrifle", 64, 2, 1 },
	{ "Sound6", "dsmislht", 64, 2, 1 },
	{ "Sound7", "dsbarexp", 32, 2, 1 },
	{ "Sound8", "dsflburn", 64, 2, 1 },
	{ "Sound9", "dsflidl", 118, 2, 1 },
	{ "Sound10", "dsagrsee", 98, 2, 1 },
	{ "Sound11", "dsplpain", 96, 2, 1 },
	{ "Sound12", "dspcrush", 96, 2, 1 },
	{ "Sound13", "dspespna", 98, 2, 1 },
	{ "Sound14", "dspespnb", 98, 2, 1 },
	{ "Sound15", "dspespnc", 98, 2, 1 },
	{ "Sound16", "dspespnd", 98, 2, 1 },
	{ "Sound17", "dsagrdpn", 98, 2, 1 },
	{ "Sound18", "dspldeth", 32, 2, 1 },
	{ "Sound19", "dsplxdth", 32, 2, 1 },
	{ "Sound20", "dsslop", 78, 2, 1 },
	{ "Sound21", "dsrebdth", 98, 2, 1 },
	{ "Sound22", "dsagrdth", 98, 2, 1 },
	{ "Sound23", "dslgfire", 211, 2, 1 },
	{ "Sound24", "dssmfire", 211, 2, 1 },
	{ "Sound25", "dsalarm", 210, 2, 1 },
	{ "Sound26", "dsdrlmto", 98, 2, 1 },
	{ "Sound27", "dsdrlmtc", 98, 2, 1 },
	{ "Sound28", "dsdrsmto", 98, 2, 1 },
	{ "Sound29", "dsdrsmtc", 98, 2, 1 },
	{ "Sound30", "dsdrlwud", 98, 2, 1 },
	{ "Sound31", "dsdrswud", 98, 2, 1 },
	{ "Sound32", "dsdrston", 98, 2, 1 },
	{ "Sound33", "dsbdopn", 98, 2, 1 },
	{ "Sound34", "dsbdcls", 98, 2, 1 },
	{ "Switch", "dsswtchn", 78, 2, 1 },
	{ "SwitchBolt", "dsswbolt", 98, 2, 1 },
	{ "SwitchScanner", "dsswscan", 98, 2, 1 },
	{ "Sound38", "dsyeah", 10, 2, 1 },
	{ "Sound39", "dsmask", 210, 2, 1 },
	{ "Sound40", "dspstart", 100, 2, 1 },
	{ "Sound41", "dspstop", 100, 2, 1 },
	{ "Sound42", "dsitemup", 78, 2, 1 },
	{ "BreakGlass", "dsbglass", 200, 2, 1 },
	{ "Sound44", "dswriver", 201, 2, 1 },
	{ "Sound45", "dswfall", 201, 2, 1 },
	{ "Sound46", "dswdrip", 201, 2, 1 },
	{ "Sound47", "dswsplsh", 95, 2, 1 },
	{ "Sound48", "dsrebact", 200, 2, 1 },
	{ "Sound49", "dsagrac1", 98, 2, 1 },
	{ "Sound50", "dsagrac2", 98, 2, 1 },
	{ "Sound51", "dsagrac3", 98, 2, 1 },
	{ "Sound52", "dsagrac4", 98, 2, 1 },
	{ "Sound53", "dsambppl", 218, 2, 1 },
	{ "Sound54", "dsambbar", 218, 2, 1 },
	{ "Sound55", "dstelept", 32, 2, 1 },
	{ "Sound56", "dsratact", 99, 2, 1 },
	{ "Sound57", "dsitmbk", 100, 2, 1 },
	{ "Sound58", "dsxbow", 99, 2, 1 },
	{ "Sound59", "dsburnme", 95, 2, 1 },
	{ "Sound60", "dsoof", 96, 2, 1 },
	{ "Sound61", "dswbrldt", 98, 2, 1 },
	{ "Sound62", "dspsdtha", 109, 2, 1 },
	{ "Sound63", "dspsdthb", 109, 2, 1 },
	{ "Sound64", "dspsdthc", 109, 2, 1 },
	{ "Sound65", "dsrb2pn", 96, 2, 1 },
	{ "Sound66", "dsrb2dth", 32, 2, 1 },
	{ "Sound67", "dsrb2see", 98, 2, 1 },
	{ "Sound68", "dsrb2act", 98, 2, 1 },
	{ "Sound69", "dsfirxpl", 70, 2, 1 },
	{ "Sound70", "dsstnmov", 100, 2, 1 },
	{ "Sound71", "dsnoway", 78, 2, 1 },
	{ "Sound72", "dsrlaunc", 64, 2, 1 },
	{ "Sound73", "dsrflite", 65, 2, 1 },
	{ "Sound74", "dsradio", 60, 2, 1 },
	{ "SwitchPullChain", "dspulchn", 98, 2, 1 },
	{ "SwitchKnob", "dsswknob", 98, 2, 1 },
	{ "SwitchKeyCard", "dskeycrd", 98, 2, 1 },
	{ "SwitchStone", "dsswston", 98, 2, 1 },
	{ "Sound79", "dssntsee", 98, 2, 1 },
	{ "Sound80", "dssntdth", 98, 2, 1 },
	{ "Sound81", "dssntact", 98, 2, 1 },
	{ "Sound82", "dspgrdat", 64, 2, 1 },
	{ "Sound83", "dspgrsee", 90, 2, 1 },
	{ "Sound84", "dspgrdpn", 96, 2, 1 },
	{ "Sound85", "dspgrdth", 32, 2, 1 },
	{ "Sound86", "dspgract", 120, 2, 1 },
	{ "Sound87", "dsproton", 64, 2, 1 },
	{ "Sound88", "dsprotfl", 64, 2, 1 },
	{ "Sound89", "dsplasma", 64, 2, 1 },
	{ "Sound90", "dsdsrptr", 30, 2, 1 },
	{ "Sound91", "dsreavat", 64, 2, 1 },
	{ "Sound92", "dsrevbld", 64, 2, 1 },
	{ "Sound93", "dsrevsee", 90, 2, 1 },
	{ "Sound94", "dsreavpn", 96, 2, 1 },
	{ "Sound95", "dsrevdth", 32, 2, 1 },
	{ "Sound96", "dsrevact", 120, 2, 1 },
	{ "Sound97", "dsspisit", 90, 2, 1 },
	{ "Sound98", "dsspdwlk", 65, 2, 1 },
	{ "Sound99", "dsspidth", 32, 2, 1 },
	{ "Sound100", "dsspdatk", 32, 2, 1 },
	{ "Sound101", "dschant", 218, 2, 1 },
	{ "Sound102", "dsstatic", 32, 2, 1 },
	{ "Sound103", "dschain", 70, 2, 1 },
	{ "Sound104", "dstend", 100, 2, 1 },
	{ "Sound105", "dsphoot", 32, 2, 1 },
	{ "Sound106", "dsexplod", 32, 2, 1 },
	{ "Sound107", "dssigil", 32, 2, 1 },
	{ "Sound108", "dssglhit", 32, 2, 1 },
	{ "Sound109", "dssiglup", 32, 2, 1 },
	{ "Sound110", "dsprgpn", 96, 2, 1 },
	{ "Sound111", "dsprogac", 120, 2, 1 },
	{ "Sound112", "dslorpn", 96, 2, 1 },
	{ "Sound113", "dslorsee", 90, 2, 1 },
	{ "SwitchFool", "dsdifool", 32, 2, 1 },
	{ "Sound115", "dsinqdth", 32, 2, 1 },
	{ "Sound116", "dsinqact", 98, 2, 1 },
	{ "Sound117", "dsinqsee", 90, 2, 1 },
	{ "Sound118", "dsinqjmp", 65, 2, 1 },
	{ "Sound119", "dsamaln1", 99, 2, 1 },
	{ "Sound120", "dsamaln2", 99, 2, 1 },
	{ "Sound121", "dsamaln3", 99, 2, 1 },
	{ "Sound122", "dsamaln4", 99, 2, 1 },
	{ "Sound123", "dsamaln5", 99, 2, 1 },
	{ "Sound124", "dsamaln6", 99, 2, 1 },
	{ "Sound125", "dsmnalse", 64, 2, 1 },
	{ "Sound126", "dsalnsee", 64, 2, 1 },
	{ "Sound127", "dsalnpn", 96, 2, 1 },
	{ "Sound128", "dsalnact", 120, 2, 1 },
	{ "Sound129", "dsalndth", 32, 2, 1 },
	{ "Sound130", "dsmnaldt", 32, 2, 1 },
	{ "Sound131", "dsreactr", 31, 2, 1 },
	{ "Sound132", "dsairlck", 98, 2, 1 },
	{ "Sound133", "dsdrchno", 98, 2, 1 },
	{ "Sound134", "dsdrchnc", 98, 2, 1 },
	{ "SwitchValve", "dsvalve", 98, 2, 1 },
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