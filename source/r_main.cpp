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
//**
//**	Rendering main loop and setup functions, utility functions (BSP,
//**  geometry, trigonometry). See tables.c, too.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"
#include "r_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void R_FreeSkyboxData();

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int						screenblocks = 0;

TVec					vieworg;
TVec					viewforward;
TVec					viewright;
TVec					viewup;	
TAVec					viewangles;

TClipPlane				view_clipplanes[4];

int						r_visframecount;

VCvarI					r_fog("r_fog", "0");
VCvarI					r_fog_test("r_fog_test", "0");
VCvarF					r_fog_r("r_fog_r", "0.5");
VCvarF					r_fog_g("r_fog_g", "0.5");
VCvarF					r_fog_b("r_fog_b", "0.5");
VCvarF					r_fog_start("r_fog_start", "1.0");
VCvarF					r_fog_end("r_fog_end", "2048.0");
VCvarF					r_fog_density("r_fog_density", "0.5");

VCvarI					r_draw_particles("r_draw_particles", "1", CVAR_Archive);

VCvarI					old_aspect("r_old_aspect_ratio", "1", CVAR_Archive);

VDrawer					*Drawer;

refdef_t				refdef;

float					PixelAspect;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static FDrawerDesc		*DrawerList[DRAWER_MAX];

static VCvarI			screen_size("screen_size", "10", CVAR_Archive);
static bool				set_resolutioon_needed = true;

// Angles in the SCREENWIDTH wide window.
static VCvarF			fov("fov", "90");
static float			old_fov = 90.0;

static int				prev_old_aspect;

static TVec				clip_base[4];

subsector_t				*r_viewleaf;
subsector_t				*r_oldviewleaf;

//
//	Translation tables
//
static VTextureTranslation*			PlayerTranslations[MAXPLAYERS + 1];
static TArray<VTextureTranslation*>	CachedTranslations;

// if true, load all graphics at start
static VCvarI			precache("precache", "1", CVAR_Archive);

static VCvarI			_driver("_driver", "0", CVAR_Rom);

// CODE --------------------------------------------------------------------

//==========================================================================
//
//  FDrawerDesc::FDrawerDesc
//
//==========================================================================

FDrawerDesc::FDrawerDesc(int Type, const char* AName, const char* ADescription,
	const char* ACmdLineArg, VDrawer* (*ACreator)())
: Name(AName)
, Description(ADescription)
, CmdLineArg(ACmdLineArg)
, Creator(ACreator)
{
	guard(FDrawerDesc::FDrawerDesc);
	DrawerList[Type] = this;
	unguard
}

//==========================================================================
//
//  R_Init
//
//==========================================================================

void R_Init()
{
	guard(R_Init);
	R_InitSkyBoxes();
	R_InitModels();
	Drawer->InitData();

	for (int i = 0; i < 256; i++)
	{
		light_remap[i] = byte(i * i / 255);
	}
	unguard;
}

//==========================================================================
//
//  R_Start
//
//==========================================================================

void R_Start(VLevel* ALevel)
{
	guard(R_Start);
	ALevel->RenderData = new VRenderLevel(ALevel);
	unguard;
}

//==========================================================================
//
//	VRenderLevel::VRenderLevel
//
//==========================================================================

VRenderLevel::VRenderLevel(VLevel* ALevel)
: Level(ALevel)
, c_subdivides(0)
, free_wsurfs(0)
, c_seg_div(0)
, AllocatedWSurfBlocks(0)
, AllocatedSubRegions(0)
, AllocatedDrawSegs(0)
, AllocatedSegParts(0)
, CurrentSky1Texture(-1)
, CurrentSky2Texture(-1)
, CurrentDoubleSky(false)
, CurrentLightning(false)
, LightningLightLevels(0)
, Particles(0)
, ActiveParticles(0)
, FreeParticles(0)
{
	guard(VRenderLevel::VRenderLevel);
	r_oldviewleaf = NULL;

	memset(DLights, 0, sizeof(DLights));
	memset(trans_sprites, 0, sizeof(trans_sprites));

	VisSize = (Level->NumSubsectors + 7) >> 3;
	BspVis = new vuint8[VisSize];

	InitParticles();
	ClearParticles();

	screenblocks = 0;

	Drawer->NewMap();

	// preload graphics
	if (precache)
	{
		PrecacheLevel();
	}
	unguard;
}

//==========================================================================
//
//	VRenderLevel::~VRenderLevel
//
//==========================================================================

VRenderLevel::~VRenderLevel()
{
	guard(VRenderLevel::~VRenderLevel);
	//	Free fake floor data.
	for (int i = 0; i < Level->NumSectors; i++)
	{
		if (Level->Sectors[i].fakefloors)
		{
			delete Level->Sectors[i].fakefloors;
		}
	}

	for (int i = 0; i < Level->NumSubsectors; i++)
	{
		for (subregion_t* r = Level->Subsectors[i].regions; r; r = r->next)
		{
			FreeSurfaces(r->floor->surfs);
			delete r->floor;
			FreeSurfaces(r->ceil->surfs);
			delete r->ceil;
		}
	}

	//	Free seg parts.
	for (int i = 0; i < Level->NumSegs; i++)
	{
		for (drawseg_t* ds = Level->Segs[i].drawsegs; ds; ds = ds->next)
		{
			FreeSegParts(ds->top);
			FreeSegParts(ds->mid);
			FreeSegParts(ds->bot);
			FreeSegParts(ds->topsky);
			FreeSegParts(ds->extra);
		}
	}
	//	Free allocated wall surface blocks.
	for (void* Block = AllocatedWSurfBlocks; Block;)
	{
		void* Next = *(void**)Block;
		Z_Free(Block);
		Block = Next;
	}
	AllocatedWSurfBlocks = NULL;

	//	Free big blocks.
	delete[] AllocatedSubRegions;
	delete[] AllocatedDrawSegs;
	delete[] AllocatedSegParts;

	if (LightningLightLevels)
	{
		delete[] LightningLightLevels;
		LightningLightLevels = NULL;
	}

	delete[] Particles;
	delete[] BspVis;

	for (int i = 0; i < SideSkies.Num(); i++)
	{
		delete SideSkies[i];
	}
	unguard;
}

//==========================================================================
//
// 	R_SetViewSize
//
// 	Do not really change anything here, because it might be in the middle
// of a refresh. The change will take effect next refresh.
//
//==========================================================================

void R_SetViewSize(int blocks)
{
	guard(R_SetViewSize);
	if (blocks > 2)
	{
		screen_size = blocks;
	}
	set_resolutioon_needed = true;
	unguard;
}

//==========================================================================
//
//  COMMAND SizeDown
//
//==========================================================================

COMMAND(SizeDown)
{
	R_SetViewSize(screenblocks - 1);
	GAudio->PlaySound(GSoundManager->GetSoundID("menu/change"),
		TVec(0, 0, 0), TVec(0, 0, 0), 0, 0, 1, 0, false);
}

//==========================================================================
//
//  COMMAND SizeUp
//
//==========================================================================

COMMAND(SizeUp)
{
	R_SetViewSize(screenblocks + 1);
	GAudio->PlaySound(GSoundManager->GetSoundID("menu/change"),
		TVec(0, 0, 0), TVec(0, 0, 0), 0, 0, 1, 0, false);
}

//==========================================================================
//
//	VRenderLevel::ExecuteSetViewSize
//
//==========================================================================

void VRenderLevel::ExecuteSetViewSize()
{
	guard(VRenderLevel::ExecuteSetViewSize);
	set_resolutioon_needed = false;
	if (screen_size < 3)
	{
		screen_size = 3;
	}
	if (screen_size > 11)
	{
		screen_size = 11;
	}
	screenblocks = screen_size;

	if (fov < 5.0)
	{
		fov = 5.0;
	}
	if (fov > 175.0)
	{
		fov = 175.0;
	}
	old_fov = fov;

	if (screenblocks > 10)
	{
		refdef.width = ScreenWidth;
		refdef.height = ScreenHeight;
		refdef.y = 0;
	}
	else
	{
		refdef.width = screenblocks * ScreenWidth / 10;
		refdef.height = (screenblocks * (ScreenHeight - SB_REALHEIGHT) / 10);
		refdef.y = (ScreenHeight - SB_REALHEIGHT - refdef.height) >> 1;
	}
	refdef.x = (ScreenWidth - refdef.width) >> 1;

	if (old_aspect)
		PixelAspect = ((float)ScreenHeight * 320.0) / ((float)ScreenWidth * 200.0);
	else
		PixelAspect = ((float)ScreenHeight * 4.0) / ((float)ScreenWidth * 3.0);
	prev_old_aspect = old_aspect;

	refdef.fovx = tan(DEG2RAD(fov) / 2);
	refdef.fovy = refdef.fovx * refdef.height / refdef.width / PixelAspect;

	// left side clip
	clip_base[0] = Normalise(TVec(1, 1.0 / refdef.fovx, 0));
	
	// right side clip
	clip_base[1] = Normalise(TVec(1, -1.0 / refdef.fovx, 0));
	
	// top side clip
	clip_base[2] = Normalise(TVec(1, 0, -1.0 / refdef.fovy));
	
	// bottom side clip
	clip_base[3] = Normalise(TVec(1, 0, 1.0 / refdef.fovy));

	refdef.drawworld = true;
	unguard;
}

//==========================================================================
//
//	R_DrawViewBorder
//
//==========================================================================

void R_DrawViewBorder()
{
	guard(R_DrawViewBorder);
	GClGame->eventDrawViewBorder(320 - screenblocks * 32,
		(480 - sb_height - screenblocks * (480 - sb_height) / 10) / 2,
		screenblocks * 64, screenblocks * (480 - sb_height) / 10);
	unguard;
}

//==========================================================================
//
//	VRenderLevel::TransformFrustum
//
//==========================================================================

void VRenderLevel::TransformFrustum()
{
	guard(VRenderLevel::TransformFrustum);
	for (int i = 0; i < 4; i++)
	{
		TVec &v = clip_base[i];
		TVec v2;

		v2.x = v.y * viewright.x + v.z * viewup.x + v.x * viewforward.x;
		v2.y = v.y * viewright.y + v.z * viewup.y + v.x * viewforward.y;
		v2.z = v.y * viewright.z + v.z * viewup.z + v.x * viewforward.z;

		view_clipplanes[i].Set(v2, DotProduct(vieworg, v2));

		view_clipplanes[i].next = i == 3 ? NULL : &view_clipplanes[i + 1];
		view_clipplanes[i].clipflag = 1 << i;
	}
	unguard;
}

//==========================================================================
//
//	VRenderLevel::SetupFrame
//
//==========================================================================

VCvarI			r_chasecam("r_chasecam", "0", CVAR_Archive);
VCvarF			r_chase_dist("r_chase_dist", "32.0", CVAR_Archive);
VCvarF			r_chase_up("r_chase_up", "32.0", CVAR_Archive);
VCvarF			r_chase_right("r_chase_right", "0", CVAR_Archive);
VCvarI			r_chase_front("r_chase_front", "0", CVAR_Archive);

void VRenderLevel::SetupFrame()
{
	guard(VRenderLevel::SetupFrame);
	// change the view size if needed
	if (screen_size != screenblocks || !screenblocks ||
		set_resolutioon_needed || old_fov != fov ||
		old_aspect != prev_old_aspect)
	{
		ExecuteSetViewSize();
	}

	ViewEnt = cl->Camera;
	viewangles = cl->ViewAngles;
	if (r_chasecam && r_chase_front)
	{
		//	This is used to see how weapon looks in player's hands
		viewangles.yaw = AngleMod(viewangles.yaw + 180);
		viewangles.pitch = -viewangles.pitch;
	}
	AngleVectors(viewangles, viewforward, viewright, viewup);

	if (r_chasecam && cl->MO == cl->Camera)
	{
		vieworg = cl->MO->Origin + TVec(0.0, 0.0, 32.0)
			- r_chase_dist * viewforward + r_chase_up * viewup
			+ r_chase_right * viewright;
	}
	else
	{
		vieworg = cl->ViewOrg;
	}

	TransformFrustum();

	ExtraLight = ViewEnt->Player ? ViewEnt->Player->ExtraLight * 8 : 0;
	if (cl->Camera == cl->MO)
	{
		ColourMap = CM_Default;
		if (cl->FixedColourmap == INVERSECOLOURMAP)
		{
			ColourMap = CM_Inverse;
			FixedLight = 255;
		}
		else if (cl->FixedColourmap == GOLDCOLOURMAP)
		{
			ColourMap = CM_Gold;
			FixedLight = 255;
		}
		else if (cl->FixedColourmap == REDCOLOURMAP)
		{
			ColourMap = CM_Red;
			FixedLight = 255;
		}
		else if (cl->FixedColourmap == GREENCOLOURMAP)
		{
			ColourMap = CM_Green;
			FixedLight = 255;
		}
		else if (cl->FixedColourmap >= NUMCOLOURMAPS)
		{
			FixedLight = 255;
		}
		else if (cl->FixedColourmap)
		{
			FixedLight = 255 - (cl->FixedColourmap << 3);
		}
		else
		{
			FixedLight = 0;
		}
	}
	else
	{
		FixedLight = 0;
		ColourMap = 0;
	}
	//	Inverse colourmap flash effect.
	if (cl->ExtraLight == 255)
	{
		ExtraLight = 0;
		ColourMap = CM_Inverse;
		FixedLight = 255;
	}

	r_viewleaf = Level->PointInSubsector(vieworg);

	Drawer->SetupView(this, &refdef);
	unguard;
}

//==========================================================================
//
//	VRenderLevel::SetupCameraFrame
//
//==========================================================================

void VRenderLevel::SetupCameraFrame(VEntity* Camera, VTexture* Tex, int FOV,
	refdef_t* rd)
{
	guard(VRenderLevel::SetupCameraFrame);
	rd->width = Tex->GetWidth();
	rd->height = Tex->GetHeight();
	rd->y = 0;
	rd->x = 0;

	if (old_aspect)
		PixelAspect = ((float)rd->height * 320.0) / ((float)rd->width * 200.0);
	else
		PixelAspect = ((float)rd->height * 4.0) / ((float)rd->width * 3.0);

	rd->fovx = tan(DEG2RAD(FOV) / 2);
	rd->fovy = rd->fovx * rd->height / rd->width / PixelAspect;

	// left side clip
	clip_base[0] = Normalise(TVec(1, 1.0 / rd->fovx, 0));
	
	// right side clip
	clip_base[1] = Normalise(TVec(1, -1.0 / rd->fovx, 0));
	
	// top side clip
	clip_base[2] = Normalise(TVec(1, 0, -1.0 / rd->fovy));
	
	// bottom side clip
	clip_base[3] = Normalise(TVec(1, 0, 1.0 / rd->fovy));

	rd->drawworld = true;

	ViewEnt = Camera;
	viewangles = Camera->Angles;
	AngleVectors(viewangles, viewforward, viewright, viewup);

	vieworg = Camera->Origin;

	TransformFrustum();

	ExtraLight = 0;
	FixedLight = 0;
	ColourMap = 0;

	r_viewleaf = Level->PointInSubsector(vieworg);

	Drawer->SetupView(this, rd);
	set_resolutioon_needed = true;
	unguard;
}

//==========================================================================
//
//	VRenderLevel::MarkLeaves
//
//==========================================================================

void VRenderLevel::MarkLeaves()
{
	guard(VRenderLevel::MarkLeaves);
	byte	*vis;
	node_t	*node;
	int		i;

	if (r_oldviewleaf == r_viewleaf)
		return;
	
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	vis = Level->LeafPVS(r_viewleaf);

	for (i = 0; i < Level->NumSubsectors; i++)
	{
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			subsector_t *sub = &Level->Subsectors[i];
			sub->VisFrame = r_visframecount;
			node = sub->parent;
			while (node)
			{
				if (node->VisFrame == r_visframecount)
					break;
				node->VisFrame = r_visframecount;
				node = node->parent;
			}
		}
	}
	unguard;
}

//**************************************************************************
//**
//**	PARTICLES
//**
//**************************************************************************

//==========================================================================
//
//	VRenderLevel::InitParticles
//
//==========================================================================

void VRenderLevel::InitParticles()
{
	guard(VRenderLevel::InitParticles);
	const char* p = GArgs.CheckValue("-particles");

	if (p)
	{
		NumParticles = atoi(p);
		if (NumParticles < ABSOLUTE_MIN_PARTICLES)
			NumParticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		NumParticles = MAX_PARTICLES;
	}

	Particles = new particle_t[NumParticles];
	unguard;
}

//==========================================================================
//
//	VRenderLevel::ClearParticles
//
//==========================================================================

void VRenderLevel::ClearParticles()
{
	guard(VRenderLevel::ClearParticles);
	FreeParticles = &Particles[0];
	ActiveParticles = NULL;

	for (int i = 0; i < NumParticles; i++)
		Particles[i].next = &Particles[i + 1];
	Particles[NumParticles - 1].next = NULL;
	unguard;
}

//==========================================================================
//
//	VRenderLevel::NewParticle
//
//==========================================================================

particle_t* VRenderLevel::NewParticle()
{
	guard(VRenderLevel::NewParticle);
	if (!FreeParticles)
	{
		//	No free particles
		return NULL;
	}
	//	Remove from list of free particles
	particle_t* p = FreeParticles;
	FreeParticles = p->next;
	//	Clean
	memset(p, 0, sizeof(*p));
	//	Add to active particles
	p->next = ActiveParticles;
	ActiveParticles = p;
	return p;
	unguard;
}

//==========================================================================
//
//	VRenderLevel::UpdateParticles
//
//==========================================================================

void VRenderLevel::UpdateParticles(float frametime)
{
	guard(VRenderLevel::UpdateParticles);
	particle_t		*p, *kill;

	kill = ActiveParticles;
	while (kill && kill->die < Level->Time)
	{
		ActiveParticles = kill->next;
		kill->next = FreeParticles;
		FreeParticles = kill;
		kill = ActiveParticles;
	}

	for (p = ActiveParticles; p; p = p->next)
	{
		kill = p->next;
		while (kill && kill->die < Level->Time)
		{
			p->next = kill->next;
			kill->next = FreeParticles;
			FreeParticles = kill;
			kill = p->next;
		}

		p->org += (p->vel * frametime);
		Level->LevelInfo->eventUpdateParticle(p, frametime);
	}
	unguard;
}

//==========================================================================
//
//	VRenderLevel::DrawParticles
//
//==========================================================================

void VRenderLevel::DrawParticles()
{
	guard(VRenderLevel::DrawParticles);
	if (!r_draw_particles)
	{
		return;
	}
	Drawer->StartParticles();
	for (particle_t* p = ActiveParticles; p; p = p->next)
	{
		if (ColourMap)
		{
			vuint32 Col = p->colour;
			rgba_t TmpCol = ColourMaps[ColourMap].GetPalette()[R_LookupRGB(
				(Col >> 16) & 0xff, (Col >> 8) & 0xff, Col & 0xff)];
			p->colour = (Col & 0xff000000) | (TmpCol.r << 16) |
				(TmpCol.g << 8) | TmpCol.b;
			Drawer->DrawParticle(p);
			p->colour = Col;
		}
		else
		{
			Drawer->DrawParticle(p);
		}
	}
	Drawer->EndParticles();
	unguard;
}

//==========================================================================
//
//  R_RenderPlayerView
//
//==========================================================================

void R_RenderPlayerView()
{
	guard(R_RenderPlayerView);
	((VRenderLevel*)GClLevel->RenderData)->RenderPlayerView();
	unguard;
}

//==========================================================================
//
//  VRenderLevel::RenderPlayerView
//
//==========================================================================

void VRenderLevel::RenderPlayerView()
{
	guard(VRenderLevel::RenderPlayerView);
	if (!Level->LevelInfo)
	{
		return;
	}

	GTextureManager.Time = Level->Time;

	BuildPlayerTranslations();

	AnimateSky(host_frametime);

	UpdateParticles(host_frametime);

	//	Update camera textures that were visible in last frame.
	for (int i = 0; i < Level->CameraTextures.Num(); i++)
	{
		UpdateCameraTexture(Level->CameraTextures[i].Camera,
			Level->CameraTextures[i].TexNum, Level->CameraTextures[i].FOV);
	}

	SetupFrame();

	MarkLeaves();

	PushDlights();

	UpdateWorld(&refdef);

	RenderWorld(&refdef);

	RenderMobjs();

	DrawParticles();

	DrawTranslucentPolys();

	// draw the psprites on top of everything
	if (fov <= 90.0 && cl->MO == cl->Camera && !host_titlemap)
	{
		DrawPlayerSprites();
	}

	Drawer->EndView();

	// Draw croshair
	if (cl->MO == cl->Camera && !host_titlemap)
	{
		DrawCroshair();
	}
	unguard;
}

//==========================================================================
//
//	VRenderLevel::UpdateCameraTexture
//
//==========================================================================

void VRenderLevel::UpdateCameraTexture(VEntity* Camera, int TexNum, int FOV)
{
	guard(VRenderLevel::UpdateCameraTexture);
	if (!Camera)
	{
		return;
	}

	if (!GTextureManager[TexNum]->bIsCameraTexture)
	{
		return;
	}
	VCameraTexture* Tex = (VCameraTexture*)GTextureManager[TexNum];
	if (!Tex->bNeedsUpdate)
	{
		return;
	}

	refdef_t		CameraRefDef;
	CameraRefDef.DrawCamera = true;

	SetupCameraFrame(Camera, Tex, FOV, &CameraRefDef);

	MarkLeaves();

	PushDlights();

	UpdateWorld(&CameraRefDef);

	RenderWorld(&CameraRefDef);

	RenderMobjs();

	DrawParticles();

	DrawTranslucentPolys();

	Drawer->EndView();

	Tex->CopyImage();
	unguard;
}

//==========================================================================
//
//	VRenderLevel::GetFade
//
//==========================================================================

vuint32 VRenderLevel::GetFade(subsector_t* Sub)
{
	guard(VRenderLevel::GetFade);
	if (r_fog_test)
	{
		return 0xff000000 | (int(255 * r_fog_r) << 16) |
			(int(255 * r_fog_g) << 8) | int(255 * r_fog_b);
	}
	if (Sub->sector->params.Fade)
	{
		return Sub->sector->params.Fade;
	}
	if (Level->LevelInfo->OutsideFog && Sub->sector->ceiling.pic == skyflatnum)
	{
		return Level->LevelInfo->OutsideFog;
	}
	if (Level->LevelInfo->Fade)
	{
		return Level->LevelInfo->Fade;
	}
	if (Level->LevelInfo->FadeTable == NAME_fogmap)
	{
		return 0xff7f7f7f;
	}
	return 0;
	unguard;
}

//==========================================================================
//
//	R_DrawPic
//
//==========================================================================

void R_DrawPic(int x, int y, int handle, float Alpha)
{
	guard(R_DrawPic);
	if (handle < 0)
	{
		return;
	}

	VTexture* Tex = GTextureManager(handle);
	x -= Tex->GetScaledSOffset();
	y -= Tex->GetScaledTOffset();
	Drawer->DrawPic(fScaleX * x, fScaleY * y,
		fScaleX * (x + Tex->GetScaledWidth()), fScaleY * (y + Tex->GetScaledHeight()),
		0, 0, Tex->GetWidth(), Tex->GetHeight(), Tex, NULL, Alpha);
	unguard;
}

//==========================================================================
//
// 	VRenderLevel::PrecacheLevel
//
// 	Preloads all relevant graphics for the level.
//
//==========================================================================

void VRenderLevel::PrecacheLevel()
{
	guard(VRenderLevel::PrecacheLevel);
	int			i;

	if (cls.demoplayback)
		return;

#ifdef __GNUC__
	char texturepresent[GTextureManager.GetNumTextures()];
#else
	char* texturepresent = (char*)Z_Malloc(GTextureManager.GetNumTextures());
#endif
	memset(texturepresent, 0, GTextureManager.GetNumTextures());

	for (i = 0; i < Level->NumSectors; i++)
	{
		texturepresent[Level->Sectors[i].floor.pic] = true;
		texturepresent[Level->Sectors[i].ceiling.pic] = true;
	}
	
	for (i = 0; i < Level->NumSides; i++)
	{
		texturepresent[Level->Sides[i].toptexture] = true;
		texturepresent[Level->Sides[i].midtexture] = true;
		texturepresent[Level->Sides[i].bottomtexture] = true;
	}

	// Precache textures.
	for (i = 1; i < GTextureManager.GetNumTextures(); i++)
	{
		if (texturepresent[i])
		{
			Drawer->PrecacheTexture(GTextureManager[i]);
		}
	}

#ifndef __GNUC__
	Z_Free(texturepresent);
#endif
	unguard;
}

//==========================================================================
//
//	VRenderLevel::GetTranslation
//
//==========================================================================

VTextureTranslation* VRenderLevel::GetTranslation(int TransNum)
{
	guard(VRenderLevel::GetTranslation);
	return R_GetCachedTranslation(TransNum, Level);
	unguard;
}

//==========================================================================
//
//	VRenderLevel::BuildPlayerTranslations
//
//==========================================================================

void VRenderLevel::BuildPlayerTranslations()
{
	guard(VRenderLevel::BuildPlayerTranslations);
	for (TThinkerIterator<VPlayerReplicationInfo> It(Level); It; ++It)
	{
		if (It->PlayerNum < 0 || It->PlayerNum >= MAXPLAYERS)
		{
			//	Should not happen.
			continue;
		}
		if (!It->TranslStart || !It->TranslEnd)
		{
			continue;
		}

		VTextureTranslation* Tr = PlayerTranslations[It->PlayerNum];
		if (Tr && Tr->TranslStart == It->TranslStart &&
			Tr->TranslEnd == It->TranslEnd && Tr->Colour == It->Colour)
		{
			continue;
		}

		if (!Tr)
		{
			Tr = new VTextureTranslation;
			PlayerTranslations[It->PlayerNum] = Tr;
		}
		//	Don't waste time clearing if it's the same range.
		if (Tr->TranslStart != It->TranslStart ||
			Tr->TranslEnd != It->TranslEnd)
		{
			Tr->Clear();
		}
		Tr->BuildPlayerTrans(It->TranslStart, It->TranslEnd, It->Colour);
	}
	unguard;
}

//==========================================================================
//
//	R_SetMenuPlayerTrans
//
//==========================================================================

int R_SetMenuPlayerTrans(int Start, int End, int Col)
{
	guard(R_SetMenuPlayerTrans);
	if (!Start || !End)
	{
		return 0;
	}

	VTextureTranslation* Tr = PlayerTranslations[MAXPLAYERS];
	if (Tr && Tr->TranslStart == Start && Tr->TranslEnd == End &&
		Tr->Colour == Col)
	{
		return (TRANSL_Player << TRANSL_TYPE_SHIFT) + MAXPLAYERS;
	}

	if (!Tr)
	{
		Tr = new VTextureTranslation;
		PlayerTranslations[MAXPLAYERS] = Tr;
	}
	if (Tr->TranslStart != Start || Tr->TranslEnd == End)
	{
		Tr->Clear();
	}
	Tr->BuildPlayerTrans(Start, End, Col);
	return (TRANSL_Player << TRANSL_TYPE_SHIFT) + MAXPLAYERS;
	unguard;
}

//==========================================================================
//
//	R_GetCachedTranslation
//
//==========================================================================

VTextureTranslation* R_GetCachedTranslation(int TransNum, VLevel* Level)
{
	guard(R_GetCachedTranslation);
	int Type = TransNum >> TRANSL_TYPE_SHIFT;
	int Index = TransNum & ((1 << TRANSL_TYPE_SHIFT) - 1);
	VTextureTranslation* Tr;
	switch (Type)
	{
	case TRANSL_Standard:
		if (Index == 7)
		{
			Tr = &IceTranslation;
		}
		else
		{
			if (Index < 0 || Index >= NumTranslationTables)
			{
				return NULL;
			}
			Tr = TranslationTables[Index];
		}
		break;

	case TRANSL_Player:
		if (Index < 0 || Index >= MAXPLAYERS + 1)
		{
			return NULL;
		}
		Tr = PlayerTranslations[Index];
		break;

	case TRANSL_Level:
		if (!Level || Index < 0 || Index >= Level->Translations.Num())
		{
			return NULL;
		}
		Tr = Level->Translations[Index];
		break;

	case TRANSL_BodyQueue:
		if (!Level || Index < 0 || Index >= Level->BodyQueueTrans.Num())
		{
			return NULL;
		}
		Tr = Level->BodyQueueTrans[Index];
		break;

	case TRANSL_Decorate:
		if (Index < 0 || Index >= DecorateTranslations.Num())
		{
			return NULL;
		}
		Tr = DecorateTranslations[Index];
		break;

	default:
		return NULL;
	}

	if (!Tr)
	{
		return NULL;
	}

	for (int i = 0; i < CachedTranslations.Num(); i++)
	{
		VTextureTranslation* Check = CachedTranslations[i];
		if (Check->Crc != Tr->Crc)
		{
			continue;
		}
		if (memcmp(Check->Palette, Tr->Palette, sizeof(Tr->Palette)))
		{
			continue;
		}
		return Check;
	}

	VTextureTranslation* Copy = new VTextureTranslation;
	*Copy = *Tr;
	CachedTranslations.Append(Copy);
	return Copy;
	unguard;
}

//==========================================================================
//
//	COMMAND TimeRefresh
//
//	For program optimization
//
//==========================================================================

COMMAND(TimeRefresh)
{
	guard(COMMAND TimeRefresh);
	int			i;
	double		start, stop, time, RenderTime, UpdateTime;
	float		startangle;

	if (!cl)
	{
		return;
	}

	startangle = cl->ViewAngles.yaw;

	RenderTime = 0;
	UpdateTime = 0;
	start = Sys_Time();
	for (i = 0; i < 128; i++)
	{
		cl->ViewAngles.yaw = (float)(i) * 360.0 / 128.0;

		Drawer->StartUpdate();

		RenderTime -= Sys_Time();
		R_RenderPlayerView();
		RenderTime += Sys_Time();

		UpdateTime -= Sys_Time();
		Drawer->Update();
		UpdateTime += Sys_Time();
	}
	stop = Sys_Time();
	time = stop - start;
	GCon->Logf("%f seconds (%f fps)", time, 128 / time);
	GCon->Logf("Render time %f, update time %f", RenderTime, UpdateTime);

	cl->ViewAngles.yaw = startangle;
	unguard;
}

//==========================================================================
//
//	V_Init
//
//==========================================================================

void V_Init()
{
	guard(V_Init);
	int DIdx = -1;
	for (int i = 0; i < DRAWER_MAX; i++)
	{
		if (!DrawerList[i])
			continue;
		//	Pick first available as default.
		if (DIdx == -1)
			DIdx = i;
		//	Check for user driver selection.
		if (DrawerList[i]->CmdLineArg && GArgs.CheckParm(DrawerList[i]->CmdLineArg))
			DIdx = i;
	}
	if (DIdx == -1)
		Sys_Error("No drawers are available");
	_driver = DIdx;
	GCon->Logf(NAME_Init, "Selected %s", DrawerList[DIdx]->Description);
	//	Create drawer.
	Drawer = DrawerList[DIdx]->Creator();
	Drawer->Init();
	unguard;
}

//==========================================================================
//
//	V_Shutdown
//
//==========================================================================

void V_Shutdown()
{
	guard(V_Shutdown);
	if (Drawer)
	{
		Drawer->Shutdown();
		delete Drawer;
		Drawer = NULL;
	}
	R_FreeModels();
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		if (PlayerTranslations[i])
		{
			delete PlayerTranslations[i];
			PlayerTranslations[i] = NULL;
		}
	}
	for (int i = 0; i < CachedTranslations.Num(); i++)
	{
		delete CachedTranslations[i];
	}
	CachedTranslations.Clear();
	R_FreeSkyboxData();
	unguard;
}

//==========================================================================
//
//	VPortal::VPortal
//
//==========================================================================

VPortal::VPortal(class VRenderLevel* ARLev)
: RLev(ARLev)
{
}

//==========================================================================
//
//	VPortal::~VPortal
//
//==========================================================================

VPortal::~VPortal()
{
}

//==========================================================================
//
//	VPortal::NeedsDepthBuffer
//
//==========================================================================

bool VPortal::NeedsDepthBuffer()
{
	return false;
}

//==========================================================================
//
//	VPortal::MatchSky
//
//==========================================================================

bool VPortal::MatchSky(VSky*)
{
	return false;
}

//==========================================================================
//
//	VPortal::MatchSkyBox
//
//==========================================================================

bool VPortal::MatchSkyBox(VEntity*)
{
	return false;
}
