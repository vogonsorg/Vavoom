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

//==========================================================================
//
// 	Map level types.
//
// 	The following data structures define the persistent format
// used in the lumps of the WAD files.
//
//==========================================================================

#pragma pack(push, 1)

// Indicates a GL-specific vertex
#define	GL_VERTEX			0x8000
#define	GL_VERTEX_V3		0x40000000
#define	GL_VERTEX_V5		0x80000000

#define GL_SEG_FLAG_SIDE	0x0001

#define GL_V2_MAGIC			"gNd2"
#define GL_V3_MAGIC			"gNd3"
#define GL_V5_MAGIC			"gNd5"

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
	ML_LABEL,		// A separator, name, ExMx or MAPxx
	ML_THINGS,		// Monsters, items..
	ML_LINEDEFS,	// LineDefs, from editing
	ML_SIDEDEFS,	// SideDefs, from editing
	ML_VERTEXES,	// Vertices, edited and BSP splits generated
	ML_SEGS,		// LineSegs, from LineDefs split by BSP
	ML_SSECTORS,	// SubSectors, list of LineSegs
	ML_NODES,		// BSP nodes
	ML_SECTORS,		// Sectors, from editing
	ML_REJECT,		// LUT, sector-sector visibility
	ML_BLOCKMAP,	// LUT, motion clipping, walls/grid element
	ML_BEHAVIOR		// ACS scripts
};

//	Lump order from "GL-Friendly Nodes" specs.
enum
{
	ML_GL_LABEL,	// A separator name, GL_ExMx or GL_MAPxx
	ML_GL_VERT,		// Extra Vertices
	ML_GL_SEGS,		// Segs, from linedefs & minisegs
	ML_GL_SSECT,	// SubSectors, list of segs
	ML_GL_NODES,	// GL BSP nodes
	ML_GL_PVS		// Potentially visible set
};

// A single Vertex.
struct mapvertex_t
{
	short		x;
	short		y;
};

//	Vertex in "GL-friendly Nodes" specification version 2.0
struct gl_mapvertex_t
{
	int 		x;
	int 		y;
};

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
struct mapsidedef_t
{
	short		textureoffset;
	short		rowoffset;
	char		toptexture[8];
	char		bottomtexture[8];
	char		midtexture[8];
	short		sector;             // Front sector, towards viewer.
};

// A LineDef, as used for editing, and as input
// to the BSP builder.
//  Doom and Heretic linedef
struct maplinedef1_t
{
	short		v1;
	short		v2;
	short		flags;
	short		special;
	short		tag;
	short		sidenum[2];	// sidenum[1] will be -1 if one sided
};

//	Hexen linedef
struct maplinedef2_t
{
	short		v1;
	short 		v2;
	short 		flags;
	byte 		special;
	byte 		arg1;
	byte 		arg2;
	byte 		arg3;
	byte 		arg4;
	byte 		arg5;
	short 		sidenum[2];	// sidenum[1] will be -1 if one sided
};

// Sector definition, from editing.
struct mapsector_t
{
	short		floorheight;
	short		ceilingheight;
	char		floorpic[8];
	char		ceilingpic[8];
	short		lightlevel;
	short		special;
	short		tag;
};

// SubSector, as generated by BSP.
struct mapsubsector_t
{
	short		numsegs;
	short		firstseg;	// Index of first one, segs are stored sequentially.
};

// SubSector, as generated by BSP.
struct mapglsubsector_v3_t
{
	int			numsegs;
	int			firstseg;	// Index of first one, segs are stored sequentially.
};

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
struct mapseg_t
{
	short		v1;
	short		v2;
	short		angle;
	short		linedef;
	short		side;
	short		offset;
};

//	Seg in "GL-friendly Nodes"
struct mapglseg_t
{
	unsigned short	v1;
	unsigned short	v2;
	short			linedef;	// -1 for minisegs
	short			side;
	short			partner;	// -1 on one-sided walls
};

//	Seg in "GL-friendly Nodes"
struct mapglseg_v3_t
{
	unsigned int	v1;
	unsigned int	v2;
	short			linedef;	// -1 for minisegs
	short			flags;
	int				partner;	// -1 on one-sided walls
};

// BSP node structure.
struct mapnode_t
{
	// Partition line from (x,y) to (x+dx,y+dy)
	short		x;
	short		y;
	short		dx;
	short		dy;

	// Bounding box for each child,
	// clip against view frustum.
	short		bbox[2][4];

	// If NF_SUBSECTOR its a subsector,
	// else it's a node of another subtree.
	unsigned short	children[2];
};

// BSP node structure.
struct mapglnode_v5_t
{
	// Partition line from (x,y) to (x+dx,y+dy)
	short		x;
	short		y;
	short		dx;
	short		dy;

	// Bounding box for each child,
	// clip against view frustum.
	short		bbox[2][4];

	// If NF_SUBSECTOR its a subsector,
	// else it's a node of another subtree.
	unsigned int	children[2];
};

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
// 	Doom and Heretic thing
struct mapthing1_t
{
	short		x;
	short		y;
	short		angle;
	short		type;
	short		options;
};

//	Hexen thing
struct mapthing2_t
{
	short 		tid;
	short 		x;
	short 		y;
	short 		height;
	short 		angle;
	short 		type;
	short 		options;
	byte 		special;
	byte 		arg1;
	byte 		arg2;
	byte 		arg3;
	byte 		arg4;
	byte 		arg5;
};

#pragma pack(pop)
