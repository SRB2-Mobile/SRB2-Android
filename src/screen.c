// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.c
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#include "doomdef.h"
#include "doomstat.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_clisrv.h"
#include "f_finale.h"
#include "y_inter.h" // usebuffer
#include "i_sound.h" // closed captions
#include "s_sound.h" // ditto
#include "g_game.h" // ditto
#include "p_local.h" // P_AutoPause()

#ifdef TOUCHINPUTS
#include "ts_main.h" // touchfingers, NUMTOUCHFINGERS
#endif

#if defined (USEASM) && !defined (NORUSEASM)//&& (!defined (_MSC_VER) || (_MSC_VER <= 1200))
#define RUSEASM //MSC.NET can't patch itself
#endif

// --------------------------------------------
// assembly or c drawer routines for 8bpp/16bpp
// --------------------------------------------
void (*colfunc)(void);
void (*colfuncs[COLDRAWFUNC_MAX])(void);

void (*spanfunc)(void);
void (*spanfuncs[SPANDRAWFUNC_MAX])(void);
void (*spanfuncs_npo2[SPANDRAWFUNC_MAX])(void);

// ------------------
// global video state
// ------------------
viddef_t vid;
INT32 setmodeneeded; //video mode change needed if > 0 (the mode number to set + 1)
UINT8 setrenderneeded = 0;
UINT8 renderswitcherror = 0;

static CV_PossibleValue_t scr_depth_cons_t[] = {{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}, {0, NULL}};

//added : 03-02-98: default screen mode, as loaded/saved in config
consvar_t cv_scr_width = CVAR_INIT ("scr_width", "1280", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height = CVAR_INIT ("scr_height", "800", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_depth = CVAR_INIT ("scr_depth", "16 bits", CV_SAVE, scr_depth_cons_t, NULL);
consvar_t cv_renderview = CVAR_INIT ("renderview", "On", 0, CV_OnOff, NULL);

#ifdef NATIVESCREENRES
static void SCR_ToggleNativeRes(void);
static void SCR_NativeResDivChanged(void);
static void SCR_NativeResAutoChanged(void);

static CV_PossibleValue_t nativeresdiv_cons_t[] = {{FRACUNIT, "MIN"}, {10 * FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t nativerescompare_cons_t[] = {{0, "Width"}, {1, "Height"}, {0, NULL}};

#define NATIVERESCVAR_FLAGS(name, default, possiblevalue, func, flags) CVAR_INIT (name, default, (CV_CALL | CV_SAVE | CV_NOINIT | flags), possiblevalue, func)
#define NATIVERESCVAR_CALL(name, default, possiblevalue, func) NATIVERESCVAR_FLAGS(name, default, possiblevalue, func, 0)
#define NATIVERESCVAR(name, default, possiblevalue) NATIVERESCVAR_CALL(name, default, possiblevalue, SCR_ToggleNativeRes)

consvar_t cv_nativeres = NATIVERESCVAR("nativeres", "On", CV_OnOff);
consvar_t cv_nativeresdiv = NATIVERESCVAR_FLAGS("nativeresdiv", "3", nativeresdiv_cons_t, SCR_NativeResDivChanged, CV_FLOAT);
consvar_t cv_nativeresauto = NATIVERESCVAR_CALL("nativeresauto", "On", CV_OnOff, SCR_NativeResAutoChanged);
consvar_t cv_nativeresfov = NATIVERESCVAR("nativeresfov", "On", CV_OnOff);
consvar_t cv_nativerescompare = NATIVERESCVAR("nativerescompare", "Height", nativerescompare_cons_t);

#undef NATIVERESCVAR

#endif

CV_PossibleValue_t cv_renderer_t[] = {
	{1, "Software"},
#ifdef HWRENDER
	{2, "OpenGL"},
#endif
	{0, NULL}
};

consvar_t cv_renderer = CVAR_INIT ("renderer", "Software", CV_SAVE|CV_NOLUA|CV_CALL, cv_renderer_t, SCR_ChangeRenderer);

static void SCR_ChangeFullscreen(void);

consvar_t cv_fullscreen = CVAR_INIT ("fullscreen", "Yes", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen);

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

INT32 scr_bpp; // current video mode bytes per pixel
UINT8 *scr_borderpatch; // flat used to fill the reduced view borders set at ST_Init()

#ifdef NATIVESCREENRES
float scr_resdiv = 1.0f;
INT32 scr_nativewidth, scr_nativeheight;
#endif

// =========================================================================

//  Short and Tall sky drawer, for the current color mode
void (*walldrawerfunc)(void);

boolean R_ASM = true;
boolean R_486 = false;
boolean R_586 = false;
boolean R_MMX = false;
boolean R_SSE = false;
boolean R_3DNow = false;
boolean R_MMXExt = false;
boolean R_SSE2 = false;

void SCR_SetDrawFuncs(void)
{
	//
	//  setup the right draw routines for either 8bpp or 16bpp
	//
	if (true)//vid.bpp == 1) //Always run in 8bpp. todo: remove all 16bpp code?
	{
		colfuncs[BASEDRAWFUNC] = R_DrawColumn_8;
		spanfuncs[BASEDRAWFUNC] = R_DrawSpan_8;

		colfunc = colfuncs[BASEDRAWFUNC];
		spanfunc = spanfuncs[BASEDRAWFUNC];

		colfuncs[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_TRANS] = R_DrawTranslatedColumn_8;
		colfuncs[COLDRAWFUNC_SHADE] = R_DrawShadeColumn_8;
		colfuncs[COLDRAWFUNC_SHADOWED] = R_DrawColumnShadowed_8;
		colfuncs[COLDRAWFUNC_TRANSTRANS] = R_DrawTranslatedTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn_8;
		colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS] = R_Draw2sMultiPatchTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_FOG] = R_DrawFogColumn_8;

		spanfuncs[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTED] = R_DrawTiltedSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTiltedTranslucentSpan_8;
		spanfuncs[SPANDRAWFUNC_SPLAT] = R_DrawSplat_8;
		spanfuncs[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_8;
		spanfuncs[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawTiltedSplat_8;
		spanfuncs[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawTiltedFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTiltedTranslucentFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDWATER] = R_DrawTiltedTranslucentWaterSpan_8;
		spanfuncs[SPANDRAWFUNC_FOG] = R_DrawFogSpan_8;

		// Lactozilla: Non-powers-of-two
		spanfuncs_npo2[BASEDRAWFUNC] = R_DrawSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTED] = R_DrawTiltedSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTiltedTranslucentSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_SPLAT] = R_DrawSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawTiltedSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawTiltedFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTiltedTranslucentFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDWATER] = R_DrawTiltedTranslucentWaterSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_FOG] = NULL; // Not needed

#ifdef RUSEASM
		if (R_ASM)
		{
			if (R_MMX)
			{
				colfuncs[BASEDRAWFUNC] = R_DrawColumn_8_MMX;
				//colfuncs[COLDRAWFUNC_SHADE] = R_DrawShadeColumn_8_ASM;
				//colfuncs[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn_8_ASM;
				colfuncs[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn_8_MMX;
				spanfuncs[BASEDRAWFUNC] = R_DrawSpan_8_MMX;
			}
			else
			{
				colfuncs[BASEDRAWFUNC] = R_DrawColumn_8_ASM;
				//colfuncs[COLDRAWFUNC_SHADE] = R_DrawShadeColumn_8_ASM;
				//colfuncs[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn_8_ASM;
				colfuncs[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn_8_ASM;
			}
		}
#endif
	}
/*	else if (vid.bpp > 1)
	{
		I_OutputMsg("using highcolor mode\n");
		spanfunc = basespanfunc = R_DrawSpan_16;
		transcolfunc = R_DrawTranslatedColumn_16;
		transtransfunc = R_DrawTranslucentColumn_16; // No 16bit operation for this function

		colfunc = basecolfunc = R_DrawColumn_16;
		shadecolfunc = NULL; // detect error if used somewhere..
		fuzzcolfunc = R_DrawTranslucentColumn_16;
		walldrawerfunc = R_DrawWallColumn_16;
	}*/
	else
		I_Error("unknown bytes per pixel mode %d\n", vid.bpp);
/*
	if (SCR_IsAspectCorrect(vid.width, vid.height))
		CONS_Alert(CONS_WARNING, M_GetText("Resolution is not aspect-correct!\nUse a multiple of %dx%d\n"), BASEVIDWIDTH, BASEVIDHEIGHT);
*/
}

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!(setmodeneeded || setrenderneeded) || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	// Lactozilla: Renderer switching
	if (setrenderneeded)
	{
		// stop recording movies (APNG only)
		if (setrenderneeded && (moviemode == MM_APNG))
			M_StopMovie();

		// VID_SetMode will call VID_CheckRenderer itself,
		// so no need to do this in here.
		if (!setmodeneeded)
			VID_CheckRenderer();

		vid.recalc = 1;
	}

	// Set the video mode in the video interface.
	if (setmodeneeded)
		VID_SetMode(setmodeneeded - 1);

	V_SetPalette(0);

	if (M_OnMobileMenu())
		M_CheckMobileMenuHeight();

	SCR_SetDrawFuncs();

	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
	setrenderneeded = 0;
}

// do some initial settings for the game loading screen
//
void SCR_Startup(void)
{
	const CPUInfoFlags *RCpuInfo = I_CPUInfo();
	if (!M_CheckParm("-NOCPUID") && RCpuInfo)
	{
#if defined (__i386__) || defined (_M_IX86) || defined (__WATCOMC__)
		R_486 = true;
#endif
		if (RCpuInfo->RDTSC)
			R_586 = true;
		if (RCpuInfo->MMX)
			R_MMX = true;
		if (RCpuInfo->AMD3DNow)
			R_3DNow = true;
		if (RCpuInfo->MMXExt)
			R_MMXExt = true;
		if (RCpuInfo->SSE)
			R_SSE = true;
		if (RCpuInfo->SSE2)
			R_SSE2 = true;
		CONS_Printf("CPU Info: 486: %i, 586: %i, MMX: %i, 3DNow: %i, MMXExt: %i, SSE2: %i\n", R_486, R_586, R_MMX, R_3DNow, R_MMXExt, R_SSE2);
	}

	if (M_CheckParm("-noASM"))
		R_ASM = false;
	if (M_CheckParm("-486"))
		R_486 = true;
	if (M_CheckParm("-586"))
		R_586 = true;
	if (M_CheckParm("-MMX"))
		R_MMX = true;
	if (M_CheckParm("-3DNow"))
		R_3DNow = true;
	if (M_CheckParm("-MMXExt"))
		R_MMXExt = true;

	if (M_CheckParm("-SSE"))
		R_SSE = true;
	if (M_CheckParm("-noSSE"))
		R_SSE = false;

	if (M_CheckParm("-SSE2"))
		R_SSE2 = true;

	M_SetupMemcpy();

	if (dedicated)
	{
		V_Init();
		V_SetPalette(0);
		return;
	}

	vid.modenum = 0;

	V_Init();
	V_Recalc();

	CV_RegisterVar(&cv_ticrate);
	CV_RegisterVar(&cv_constextsize);

	V_SetPalette(0);
}

// Called at new frame, if the video mode has changed
//
void SCR_Recalc(void)
{
	if (dedicated)
		return;

	// bytes per pixel quick access
	scr_bpp = vid.bpp;

	V_Recalc();

	// toggle off (then back on) the automap because some screensize-dependent values will
	// be calculated next time the automap is activated.
	if (automapactive)
	{
		am_recalc = true;
		AM_Start();
	}

	// set the screen[x] ptrs on the new vidbuffers
	V_Init();

	// scr_viewsize doesn't change, neither detailLevel, but the pixels
	// per screenblock is different now, since we've changed resolution.
	R_SetViewSize(); //just set setsizeneeded true now ..

	// vid.recalc lasts only for the next refresh...
	con_recalc = true;
	am_recalc = true;

#ifdef HWRENDER
	// Shoot! The screen texture was flushed!
	if ((rendermode == render_opengl) && (gamestate == GS_INTERMISSION))
		usebuffer = false;
#endif
}

// Check for screen cmd-line parms: to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode(void)
{
	INT32 scr_forcex, scr_forcey; // resolution asked from the cmd-line

	if (dedicated)
		return;

	// 0 means not set at the cmd-line
	scr_forcex = scr_forcey = 0;

	if (M_CheckParm("-width") && M_IsNextParm())
		scr_forcex = atoi(M_GetNextParm());

	if (M_CheckParm("-height") && M_IsNextParm())
		scr_forcey = atoi(M_GetNextParm());

#ifdef NATIVESCREENRES
	if (cv_nativeres.value)
	{
		VID_GetNativeResolution(&scr_nativewidth, &scr_nativeheight);

		if (M_CheckParm("-nativewidth") && M_IsNextParm())
			scr_nativewidth = atoi(M_GetNextParm());

		if (M_CheckParm("-nativeheight") && M_IsNextParm())
			scr_nativeheight = atoi(M_GetNextParm());

		if (scr_nativewidth || scr_nativeheight)
		{
			SCR_SetMaxNativeResDivider(SCR_GetMaxNativeResDivider());
			if (!cv_nativeresauto.value)
				SCR_ResetNativeResDivider(); // Reset to default if the native width or height changed
		}

		scr_resdiv = SCR_GetNativeResDivider(scr_nativewidth, scr_nativeheight);
	}
#endif

	if (scr_forcex && scr_forcey)
	{
		CONS_Printf(M_GetText("Using resolution: %d x %d\n"), scr_forcex, scr_forcey);
		// returns -1 if not found, thus will be 0 (no mode change) if not found
		setmodeneeded = VID_GetModeForSize(scr_forcex, scr_forcey) + 1;
	}
	else
	{
		CONS_Printf(M_GetText("Default resolution: %d x %d (%d bits)\n"), cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		// see note above
		SCR_SetModeFromConfig();
	}

	if (cv_renderer.value != (signed)rendermode)
	{
		if (chosenrendermode == render_none) // nothing set at command line
			SCR_ChangeRenderer();
		else
		{
			// Set cv_renderer to the current render mode
			CV_StealthSetValue(&cv_renderer, rendermode);
		}
	}
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	// remember the default screen size
	CV_SetValue(&cv_scr_width, vid.width);
	CV_SetValue(&cv_scr_height, vid.height);
	CV_SetValue(&cv_scr_depth, vid.bpp*8);
}

// Set the mode number based on the resolution saved in the config
void SCR_SetModeFromConfig(void)
{
	setmodeneeded = max(0, VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value)) + 1;
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen(void)
{
#ifdef DIRECTFULLSCREEN
	// allow_fullscreen is set by VID_PrepareModeList
	// it is used to prevent switching to fullscreen during startup
	if (!allow_fullscreen)
		return;

	if (graphics_started)
	{
		VID_PrepareModeList();

#ifdef NATIVESCREENRES
		if (cv_nativeres.value)
			SCR_SetModeFromConfig();
		else
#endif
		// Lactozilla: This only works with the preset video modes.
		setmodeneeded = VID_GetModeForSize(vid.width, vid.height) + 1;
	}
	return;
#endif
}

void SCR_ChangeRenderer(void)
{
	if (chosenrendermode != render_none
	|| (signed)rendermode == cv_renderer.value)
		return;

#ifdef HWRENDER
	// Check if OpenGL loaded successfully (or wasn't disabled) before switching to it.
	if ((vid.glstate == VID_GL_LIBRARY_ERROR)
	&& (cv_renderer.value == render_opengl))
	{
		if (M_CheckParm("-nogl"))
			CONS_Alert(CONS_ERROR, "OpenGL rendering was disabled!\n");
		else
		{
			renderswitcherror = render_opengl;
			CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		}

		return;
	}
#endif

	// Set the new render mode
	setrenderneeded = cv_renderer.value;
}

boolean SCR_IsAspectCorrect(INT32 width, INT32 height)
{
	return
	 (  width % BASEVIDWIDTH == 0
	 && height % BASEVIDHEIGHT == 0
	 && width / BASEVIDWIDTH == height / BASEVIDHEIGHT
	 );
}

#ifdef NATIVESCREENRES
void SCR_GetNativeResolution(INT32 *width, INT32 *height)
{
	// Lactozilla: Different from VID_GetNativeResolution in that
	// it might return overriden native resolutions
	INT32 w = 0, h = 0;

	VID_GetNativeResolution(&w, &h);

	if (width)
	{
		if (scr_nativewidth)
			*width = scr_nativewidth;
		else
			*width = w;
	}

	if (height)
	{
		if (scr_nativeheight)
			*height = scr_nativeheight;
		else
			*height = h;
	}
}

void SCR_ResetNativeResDivider(void)
{
	float resdiv = atof(cv_nativeresdiv.defaultvalue);
	char f[9];

	scr_resdiv = resdiv;

	sprintf(f, "%.6f", resdiv);
	CV_StealthSet(&cv_nativeresdiv, cv_nativeresdiv.defaultvalue);
}

static void SCR_ToggleNativeRes(void)
{
	scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
	setmodeneeded = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT) + 1;
}

static void SCR_NativeResDivChanged(void)
{
	CV_StealthSetValue(&cv_nativeresauto, 0);
	CV_StealthSetValue(&cv_nativeres, 1);
	SCR_ToggleNativeRes();
}

static void SCR_NativeResAutoChanged(void)
{
	if (cv_nativeresauto.value)
	{
		INT32 w = 0, h = 0;
		char f[9];

		// Set for next resolution change
		SCR_GetNativeResolution(&w, &h);
		scr_resdiv = SCR_GetNativeResDivider(w, h);

		// Stealth change current resolution divider variable
		sprintf(f, "%.6f", scr_resdiv);
		CV_StealthSet(&cv_nativeresdiv, f);
	}
	else
		SCR_ResetNativeResDivider();

	if (cv_nativeres.value)
		SCR_SetModeFromConfig();
}

#define RESDIVFACTOR (1.0f / 16.0f)

float SCR_GetNativeResDivider(INT32 width, INT32 height)
{
	if (cv_nativeresauto.value)
	{
		float w = (float)width;
		float h = (float)height;
		float wsize, hsize;
		float div = 1.0f;

		while (true)
		{
			wsize = (w / div);
			hsize = (h / div);

			if (wsize <= (w / 3) || hsize <= (h / 3))
				break;

			div += 1.0f;
			if (div > 10)
				break;
		}

		return div;
	}

	return FixedToFloat(cv_nativeresdiv.value);
}

float SCR_GetMaxNativeResDivider(void)
{
	INT32 nw = 0, nh = 0;
	float w, h;
	float div = 1.0f;

	SCR_GetNativeResolution(&nw, &nh);
	w = (float)nw;
	h = (float)nh;

	while (true)
	{
		w = ((float)nw / div);
		h = ((float)nh / div);
		if (w <= (INT32)BASEVIDWIDTH || h <= (INT32)BASEVIDHEIGHT)
			return div;
		div += RESDIVFACTOR;
	}

	return div;
}

void SCR_SetMaxNativeResDivider(float max)
{
	nativeresdiv_cons_t[1].value = FloatToFixed(max);
}
#endif

// XMOD FPS display
// moved out of os-specific code for consistency
static boolean fpsgraph[TICRATE];
static tic_t lasttic;

void SCR_DisplayTicRate(void)
{
	tic_t i;
	tic_t ontic = I_GetTime();
	tic_t totaltics = 0;
	INT32 ticcntcolor = 0;
	const INT32 h = vid.height-(8*vid.dupy);

	if (gamestate == GS_NULL)
		return;

	for (i = lasttic + 1; i < TICRATE+lasttic && i < ontic; ++i)
		fpsgraph[i % TICRATE] = false;

	fpsgraph[ontic % TICRATE] = true;

	for (i = 0;i < TICRATE;++i)
		if (fpsgraph[i])
			++totaltics;

	if (totaltics <= TICRATE/2) ticcntcolor = V_REDMAP;
	else if (totaltics == TICRATE) ticcntcolor = V_GREENMAP;

	if (cv_ticrate.value == 2) // compact counter
		V_DrawString(vid.width-(16*vid.dupx), h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, va("%02d", totaltics));
	else if (cv_ticrate.value == 1) // full counter
	{
		V_DrawString(vid.width-(72*vid.dupx), h,
			V_YELLOWMAP|V_NOSCALESTART|V_USERHUDTRANS, "FPS:");
		V_DrawString(vid.width-(40*vid.dupx), h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, va("%02d/%02u", totaltics, TICRATE));
	}

	lasttic = ontic;
}

void SCR_DisplayLocalPing(void)
{
	UINT32 ping = playerpingtable[consoleplayer];	// consoleplayer's ping is everyone's ping in a splitnetgame :P
	if (cv_showping.value == 1 || (cv_showping.value == 2 && servermaxping && ping > servermaxping))	// only show 2 (warning) if our ping is at a bad level
	{
		INT32 dispy = cv_ticrate.value ? 180 : 189;
		HU_drawPing(307, dispy, ping, true, V_SNAPTORIGHT | V_SNAPTOBOTTOM);
	}
}

void SCR_ClosedCaptions(void)
{
	UINT8 i;
	boolean gamestopped = (paused || P_AutoPause());
	INT32 basey = BASEVIDHEIGHT;

	if (gamestate != wipegamestate)
		return;

	if (gamestate == GS_LEVEL)
	{
		if (promptactive)
			basey -= 42;
		else if (splitscreen)
			basey -= 8;
		else if ((modeattacking == ATTACKING_NIGHTS)
		|| (!(maptol & TOL_NIGHTS)
		&& ((cv_powerupdisplay.value == 2) // "Always"
		 || (cv_powerupdisplay.value == 1 && !camera.chase)))) // "First-person only"
			basey -= 16;
	}

	for (i = 0; i < NUMCAPTIONS; i++)
	{
		INT32 flags, x, y, h = 10;
		char dot;
		boolean music;
		const char *caption;

		if (!closedcaptions[i].s)
			continue;

		music = (closedcaptions[i].s-S_sfx == sfx_None);

		if (music && !gamestopped && (closedcaptions[i].t < flashingtics) && (closedcaptions[i].t & 1))
			continue;

		flags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_ALLOWLOWERCASE;
		x = BASEVIDWIDTH - 20;
		y = basey-((i + 2)*h);

		if (closedcaptions[i].b)
			y -= (closedcaptions[i].b--)*vid.dupy;

		if (closedcaptions[i].t < CAPTIONFADETICS)
			flags |= (((CAPTIONFADETICS-closedcaptions[i].t)/2)*V_10TRANS);

		if (music)
			dot = '\x19';
		else if (closedcaptions[i].c && closedcaptions[i].c->origin)
			dot = '\x1E';
		else
			dot = ' ';

		caption = va("%c [%s]", dot, (closedcaptions[i].s->caption[0] ? closedcaptions[i].s->caption : closedcaptions[i].s->name));
		V_DrawRightAlignedString(x, y, flags, caption);
	}
}

void SCR_DisplayMarathonInfo(void)
{
	INT32 flags = V_SNAPTOBOTTOM;
	static tic_t entertic, oldentertics = 0, antisplice[2] = {48,0};
	const char *str;
#if 0 // eh, this probably isn't going to be a problem
	if (((signed)marathontime) < 0)
	{
		flags |= V_REDMAP;
		str = "No waiting out the clock to submit a bogus time.";
	}
	else
#endif
	{
		entertic = I_GetTime();
		if (gamecomplete)
			flags |= V_YELLOWMAP;
		else if (marathonmode & MA_INGAME)
			; // see also G_Ticker
		else if (marathonmode & MA_INIT)
			marathonmode &= ~MA_INIT;
		else
			marathontime += entertic - oldentertics;

		// Create a sequence of primes such that their LCM is nice and big.
#define PRIMEV1 13
#define PRIMEV2 17 // I can't believe it! I'm on TV!
		antisplice[0] += (entertic - oldentertics)*PRIMEV2;
		antisplice[0] %= PRIMEV1*((vid.width/vid.dupx)+1);
		antisplice[1] += (entertic - oldentertics)*PRIMEV1;
		antisplice[1] %= PRIMEV1*((vid.width/vid.dupx)+1);
		str = va("%i:%02i:%02i.%02i",
			G_TicsToHours(marathontime),
			G_TicsToMinutes(marathontime, false),
			G_TicsToSeconds(marathontime),
			G_TicsToCentiseconds(marathontime));
		oldentertics = entertic;
	}
	V_DrawFill((antisplice[0]/PRIMEV1)-1, BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT);
	V_DrawFill((antisplice[0]/PRIMEV1),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT|31);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)-1), BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT|31);
#undef PRIMEV1
#undef PRIMEV2
	V_DrawPromptBack(-8, cons_backcolor.value);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-8, flags, str);
}

#ifdef TOUCHINPUTS
void SCR_DisplayFingers(void)
{
	patch_t *cursor = W_CachePatchLongName("JOY_CURSOR", PU_PATCH);

	fixed_t pw = SHORT(cursor->width) * FRACUNIT;
	fixed_t ph = SHORT(cursor->height) * FRACUNIT;

	fixed_t w = 8 * max(FRACUNIT, vid.fdupx);
	fixed_t h = 8 * max(FRACUNIT, vid.fdupy);

	fixed_t xscale = FixedDiv(w, pw);
	fixed_t yscale = FixedDiv(h, ph);

	fixed_t xoffs = FixedMul(w / 2, vid.fdupx);
	fixed_t yoffs = FixedMul(h / 2, vid.fdupy);

	INT32 i;

	// generate colormap
	static UINT8 *colormap = NULL;
	size_t colsize = 256 * sizeof(UINT8);
	if (colormap == NULL)
		colormap = Z_Calloc(colsize, PU_STATIC, NULL);

	for (i = 0; i < NUMTOUCHFINGERS; i++)
	{
		touchfinger_t *finger = &touchfingers[i];

		if (finger->down)
		{
			fixed_t x = (finger->x * FRACUNIT) - xoffs;
			fixed_t y = (finger->y * FRACUNIT) - yoffs;
			V_DrawStretchyFixedPatch(x, y, xscale, yscale, (V_NOSCALESTART | V_20TRANS), cursor, colormap);
		}
	}
}
#endif
