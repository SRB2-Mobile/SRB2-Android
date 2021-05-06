// Emacs style mode select   -*- C++ -*-
// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2020 by Sonic Team Junior.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 graphics stuff for SDL

#include <stdlib.h>

#include <signal.h>

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include "SDL.h"

#if defined(__ANDROID__)
#include "SDL_rwops.h"
#endif

#ifdef _MSC_VER
#include <windows.h>
#pragma warning(default : 4214 4244)
#endif

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef HAVE_IMAGE
#include "SDL_image.h"
#elif defined (__unix__) || (!defined(__APPLE__) && defined (UNIXCOMMON)) // Windows & Mac don't need this, as SDL will do it for us.
#define LOAD_XPM //I want XPM!
#include "IMG_xpm.c" //Alam: I don't want to add SDL_Image.dll/so
#define HAVE_IMAGE //I have SDL_Image, sortof
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

#include "../doomdef.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#endif

#include "../doomstat.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_joy.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../g_game.h"
#include "../g_input.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#include "../r_main.h"
#include "../lua_hook.h"
#include "sdlmain.h"

#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"
#include "../hardware/r_glcommon/r_glcommon.h" // lastglerror
// For dynamic referencing of HW rendering functions
#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif

#ifdef TOUCHINPUTS
#include "../ts_main.h"
#endif

#if defined(SPLASH_SCREEN) && defined(HAVE_PNG)
	#include "../r_picformats.h" // zlib defines and png.h include
	#ifdef PNG_READ_SUPPORTED
		#define SPLASH_SCREEN_SUPPORTED

		static void SplashScreen_FreeImage(void);
	#endif
#endif

// maximum number of windowed modes (see windowedModes[][])
#define MAXWINMODES (18)

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_none; // set by command line arguments

boolean highcolor = false;

// synchronize page flipping with screen refresh
consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE, CV_OnOff, NULL);
static consvar_t cv_stretch = CVAR_INIT ("stretch", "Off", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL);
static consvar_t cv_alwaysgrabmouse = CVAR_INIT ("alwaysgrabmouse", "Off", CV_SAVE, CV_OnOff, NULL);

#ifdef DITHER
static void Impl_SetDither(void);
static consvar_t cv_dither = CVAR_INIT ("dither", "Off", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, Impl_SetDither);
#endif

static void Impl_SetColorBufferDepth(INT32 red, INT32 green, INT32 blue, INT32 alpha);

UINT8 graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static SDL_bool disable_fullscreen = SDL_FALSE;

#if defined(__ANDROID__)
#define USE_FULLSCREEN SDL_TRUE
#else
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
#endif

static SDL_bool disable_mouse = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && havefocus)
#define MOUSE_MENU false //(!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN)
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// Total mouse motion X/Y offsets
static      INT32        mousemovex = 0, mousemovey = 0;

// SDL vars
static      SDL_Surface *vidSurface = NULL;
static      SDL_Surface *bufSurface = NULL;
static      SDL_Surface *icoSurface = NULL;
static      SDL_Color    localPalette[256];
static       SDL_bool    mousegrabok = SDL_TRUE;
static       SDL_bool    wrapmouseok = SDL_FALSE;
#define HalfWarpMouse(x,y) if (wrapmouseok) SDL_WarpMouseInWindow(window, (Uint16)(x/2),(Uint16)(y/2))
static       SDL_bool    usesdl2soft = SDL_FALSE;
static       SDL_bool    borderlesswindow = SDL_FALSE;
static       SDL_bool    appOnBackground = SDL_FALSE;

Uint16      realwidth = BASEVIDWIDTH;
Uint16      realheight = BASEVIDHEIGHT;

static struct
{
	SDL_bool displaying;
	UINT32 *image;
} splashScreen;

// SDL2 vars
SDL_Window   *window;
SDL_Renderer *renderer;
static SDL_Texture  *texture;
static SDL_bool      havefocus = SDL_TRUE;

// windowed video modes from which to choose from.
static INT32 windowedModes[MAXWINMODES][2] =
{
	{1920,1200}, // 1.60,6.00
	{1920,1080}, // 1.66
	{1680,1050}, // 1.60,5.25
	{1600,1200}, // 1.33
	{1600, 900}, // 1.66
	{1366, 768}, // 1.66
	{1440, 900}, // 1.60,4.50
	{1280,1024}, // 1.33?
	{1280, 960}, // 1.33,4.00
	{1280, 800}, // 1.60,4.00
	{1280, 720}, // 1.66
	{1152, 864}, // 1.33,3.60
	{1024, 768}, // 1.33,3.20
	{ 800, 600}, // 1.33,2.50
	{ 640, 480}, // 1.33,2.00
	{ 640, 400}, // 1.60,2.00
	{ 320, 240}, // 1.33,1.00
	{ 320, 200}, // 1.60,1.00
};

static char vidModeName[MAXWINMODES][32];
static const char *fallback_resolution_name = "Fallback";

static void Impl_VideoSetupSDLBuffer(void);
static void Impl_VideoSetupBuffer(void);
static SDL_bool Impl_CreateWindow(SDL_bool fullscreen);
static void Impl_SetWindowIcon(void);

static void Impl_VideoSetupSoftwareSurface(INT32 width, INT32 height)
{
	int bpp = 16;
	int sw_texture_format = SDL_PIXELFORMAT_ABGR8888;

#if !defined(__ANDROID__)
	if (!usesdl2soft)
	{
		sw_texture_format = SDL_PIXELFORMAT_RGB565;
	}
	else
#endif
	{
		bpp = 32;
		sw_texture_format = SDL_PIXELFORMAT_RGBA8888;
	}

	if (texture == NULL)
		texture = SDL_CreateTexture(renderer, sw_texture_format, SDL_TEXTUREACCESS_STREAMING, width, height);

	// Set up SW surface
	if (vidSurface == NULL)
	{
		Uint32 rmask;
		Uint32 gmask;
		Uint32 bmask;
		Uint32 amask;

		SDL_PixelFormatEnumToMasks(sw_texture_format, &bpp, &rmask, &gmask, &bmask, &amask);
		vidSurface = SDL_CreateRGBSurface(0, width, height, bpp, rmask, gmask, bmask, amask);
	}
}

static void SDLSetMode(INT32 width, INT32 height, SDL_bool fullscreen, SDL_bool reposition)
{
	static SDL_bool wasfullscreen = SDL_FALSE;
	int fullscreen_type = SDL_WINDOW_FULLSCREEN_DESKTOP;

	realwidth = vid.width;
	realheight = vid.height;

	if (window)
	{
		if (fullscreen)
		{
			wasfullscreen = SDL_TRUE;
			SDL_SetWindowFullscreen(window, fullscreen_type);
		}
		else // windowed mode
		{
			if (wasfullscreen)
			{
				wasfullscreen = SDL_FALSE;
				SDL_SetWindowFullscreen(window, 0);
			}

			// Reposition window only in windowed mode
			SDL_SetWindowSize(window, width, height);

#if !defined(__ANDROID__)
			if (reposition)
			{
				SDL_SetWindowPosition(window,
					SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetWindowDisplayIndex(window)),
					SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetWindowDisplayIndex(window))
				);
			}
#else
			(void)reposition;
#endif
		}
	}
	else
	{
		Impl_CreateWindow(fullscreen);
		wasfullscreen = fullscreen;
		SDL_SetWindowSize(window, width, height);
		if (fullscreen)
			SDL_SetWindowFullscreen(window, fullscreen_type);
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		OglSdlSurface(vid.width, vid.height);
	}
#endif

	if (rendermode == render_soft)
	{
		SDL_RenderClear(renderer);
		SDL_RenderSetLogicalSize(renderer, width, height);
		// Set up Texture
		realwidth = width;
		realheight = height;
		if (texture != NULL)
		{
			SDL_DestroyTexture(texture);
			texture = NULL;
		}

		if (vidSurface != NULL)
		{
			SDL_FreeSurface(vidSurface);
			vidSurface = NULL;
		}
		if (vid.buffer)
		{
			free(vid.buffer);
			vid.buffer = NULL;
		}

		Impl_VideoSetupSoftwareSurface(width, height);
	}
}

#if defined(__ANDROID__)
static void Impl_AppEnteredForeground(void)
{
	VID_DestroyContext();
	VID_CreateContext();

	SDL_DestroyTexture(texture);
	texture = NULL;

	if (rendermode == render_soft)
	{
		Impl_VideoSetupSoftwareSurface(vid.width, vid.height);
		SDL_RenderSetLogicalSize(renderer, vid.width, vid.height);
	}
}
#endif

static INT32 Impl_SDL_Scancode_To_Keycode(SDL_Scancode code)
{
	if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z)
	{
		// get lowercase ASCII
		return code - SDL_SCANCODE_A + 'a';
	}
	if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9)
	{
		return code - SDL_SCANCODE_1 + '1';
	}
	else if (code == SDL_SCANCODE_0)
	{
		return '0';
	}
	if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10)
	{
		return KEY_F1 + (code - SDL_SCANCODE_F1);
	}
	switch (code)
	{
		// F11 and F12 are separated from the rest of the function keys
		case SDL_SCANCODE_F11: return KEY_F11;
		case SDL_SCANCODE_F12: return KEY_F12;

		case SDL_SCANCODE_KP_0: return KEY_KEYPAD0;
		case SDL_SCANCODE_KP_1: return KEY_KEYPAD1;
		case SDL_SCANCODE_KP_2: return KEY_KEYPAD2;
		case SDL_SCANCODE_KP_3: return KEY_KEYPAD3;
		case SDL_SCANCODE_KP_4: return KEY_KEYPAD4;
		case SDL_SCANCODE_KP_5: return KEY_KEYPAD5;
		case SDL_SCANCODE_KP_6: return KEY_KEYPAD6;
		case SDL_SCANCODE_KP_7: return KEY_KEYPAD7;
		case SDL_SCANCODE_KP_8: return KEY_KEYPAD8;
		case SDL_SCANCODE_KP_9: return KEY_KEYPAD9;

		case SDL_SCANCODE_RETURN:         return KEY_ENTER;
		case SDL_SCANCODE_ESCAPE:         return KEY_ESCAPE;
		case SDL_SCANCODE_BACKSPACE:      return KEY_BACKSPACE;
		case SDL_SCANCODE_TAB:            return KEY_TAB;
		case SDL_SCANCODE_SPACE:          return KEY_SPACE;
		case SDL_SCANCODE_MINUS:          return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:         return KEY_EQUALS;
		case SDL_SCANCODE_LEFTBRACKET:    return '[';
		case SDL_SCANCODE_RIGHTBRACKET:   return ']';
		case SDL_SCANCODE_BACKSLASH:      return '\\';
		case SDL_SCANCODE_NONUSHASH:      return '#';
		case SDL_SCANCODE_SEMICOLON:      return ';';
		case SDL_SCANCODE_APOSTROPHE:     return '\'';
		case SDL_SCANCODE_GRAVE:          return '`';
		case SDL_SCANCODE_COMMA:          return ',';
		case SDL_SCANCODE_PERIOD:         return '.';
		case SDL_SCANCODE_SLASH:          return '/';
		case SDL_SCANCODE_CAPSLOCK:       return KEY_CAPSLOCK;
		case SDL_SCANCODE_PRINTSCREEN:    return 0; // undefined?
		case SDL_SCANCODE_SCROLLLOCK:     return KEY_SCROLLLOCK;
		case SDL_SCANCODE_PAUSE:          return KEY_PAUSE;
		case SDL_SCANCODE_INSERT:         return KEY_INS;
		case SDL_SCANCODE_HOME:           return KEY_HOME;
		case SDL_SCANCODE_PAGEUP:         return KEY_PGUP;
		case SDL_SCANCODE_DELETE:         return KEY_DEL;
		case SDL_SCANCODE_END:            return KEY_END;
		case SDL_SCANCODE_PAGEDOWN:       return KEY_PGDN;
		case SDL_SCANCODE_RIGHT:          return KEY_RIGHTARROW;
		case SDL_SCANCODE_LEFT:           return KEY_LEFTARROW;
		case SDL_SCANCODE_DOWN:           return KEY_DOWNARROW;
		case SDL_SCANCODE_UP:             return KEY_UPARROW;
		case SDL_SCANCODE_NUMLOCKCLEAR:   return KEY_NUMLOCK;
		case SDL_SCANCODE_KP_DIVIDE:      return KEY_KPADSLASH;
		case SDL_SCANCODE_KP_MULTIPLY:    return '*'; // undefined?
		case SDL_SCANCODE_KP_MINUS:       return KEY_MINUSPAD;
		case SDL_SCANCODE_KP_PLUS:        return KEY_PLUSPAD;
		case SDL_SCANCODE_KP_ENTER:       return KEY_ENTER;
		case SDL_SCANCODE_KP_PERIOD:      return KEY_KPADDEL;
		case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

		case SDL_SCANCODE_LSHIFT: return KEY_LSHIFT;
		case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;
		case SDL_SCANCODE_LCTRL:  return KEY_LCTRL;
		case SDL_SCANCODE_RCTRL:  return KEY_RCTRL;
		case SDL_SCANCODE_LALT:   return KEY_LALT;
		case SDL_SCANCODE_RALT:   return KEY_RALT;
		case SDL_SCANCODE_LGUI:   return KEY_LEFTWIN;
		case SDL_SCANCODE_RGUI:   return KEY_RIGHTWIN;

#if defined(__ANDROID__)
		case SDL_SCANCODE_AC_BACK: return KEY_ESCAPE;
#endif

		default:                  break;
	}

	return 0;
}

static boolean IgnoreMouse(void)
{
	if (cv_alwaysgrabmouse.value)
		return false;
	if (menuactive)
		return !M_MouseNeeded();
	if (paused || con_destlines || chat_on)
		return true;
	if (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION &&
			gamestate != GS_CONTINUING && gamestate != GS_CUTSCENE)
		return true;
	return false;
}

static void SDLdoGrabMouse(void)
{
#if !defined(__ANDROID__)
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetWindowGrab(window, SDL_TRUE);
#endif
	if (SDL_SetRelativeMouseMode(SDL_TRUE) == 0) // already warps mouse if successful
		wrapmouseok = SDL_TRUE; // TODO: is wrapmouseok or HalfWarpMouse needed anymore?
}

static void SDLdoUngrabMouse(void)
{
#if !defined(__ANDROID__)
	SDL_ShowCursor(SDL_ENABLE);
	SDL_SetWindowGrab(window, SDL_FALSE);
#endif
	wrapmouseok = SDL_FALSE;
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO && window != NULL)
		SDLdoUngrabMouse();
}

void I_UpdateMouseGrab(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO && window != NULL
	&& SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window
	&& USE_MOUSEINPUT && !IgnoreMouse())
		SDLdoGrabMouse();
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

// SDL2 doesn't have SDL_GetVideoSurface or a lot of the SDL_Surface flags that SDL 1.2 had
static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	INT32 vfBPP;

	if (!infoSurface)
		return;

	if (!SurfaceText)
		SurfaceText = M_GetText("Unknown Surface");

	vfBPP = infoSurface->format?infoSurface->format->BitsPerPixel:0;

	CONS_Printf("\x82" "%s\n", SurfaceText);
	CONS_Printf(M_GetText(" %ix%i at %i bit color\n"), infoSurface->w, infoSurface->h, vfBPP);

	if (infoSurface->flags&SDL_PREALLOC)
		CONS_Printf("%s", M_GetText(" Uses preallocated memory\n"));
	else
		CONS_Printf("%s", M_GetText(" Stored in system memory\n"));
	if (infoSurface->flags&SDL_RLEACCEL)
		CONS_Printf("%s", M_GetText(" Colorkey RLE acceleration blit\n"));
}

static void VID_Command_Info_f (void)
{
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
}

static void VID_Command_ModeList_f(void)
{
	INT32 i = 0;

#if !defined(__ANDROID__)
	CONS_Printf("NOTE: Under SDL2, all modes are supported on all platforms.\n");
	CONS_Printf("Under OpenGL, fullscreen only supports native desktop resolution.\n");
	CONS_Printf("Under software, the mode is stretched up to desktop resolution.\n");
#endif

	for (; i < MAXWINMODES; i++)
		CONS_Printf("%2d: %dx%d\n", i, windowedModes[i][0], windowedModes[i][1]);
}

static void VID_Command_Mode_f (void)
{
	INT32 modenum;

	if (COM_Argc()!= 2)
	{
		CONS_Printf(M_GetText("vid_mode <modenum> : set video mode, current video mode %i\n"), vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf(M_GetText("Video mode not present\n"));
	else
	{
#ifdef NATIVESCREENRES
		CV_StealthSetValue(&cv_nativeres, false);
#endif
		setmodeneeded = modenum + 1; // request vid mode change
	}
}

static void Impl_Unfocused(boolean unfocused)
{
	window_notinfocus = unfocused;
	CON_FocusChanged();

	if (window_notinfocus)
	{
		if (! cv_playmusicifunfocused.value)
			S_PauseAudio();
		if (! cv_playsoundsifunfocused.value)
			S_StopSounds();

		memset(gamekeydown, 0, NUMKEYS); // TODO this is a scary memset
	}
	else if (!paused)
		S_ResumeAudio();
}

static void Impl_HandleWindowEvent(SDL_WindowEvent evt)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;
	static SDL_bool mousefocus = SDL_TRUE;
	static SDL_bool kbfocus = SDL_TRUE;

	switch (evt.event)
	{
		case SDL_WINDOWEVENT_ENTER:
			mousefocus = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_LEAVE:
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			kbfocus = SDL_TRUE;
			mousefocus = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			kbfocus = SDL_FALSE;
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			break;
	}

	if (mousefocus && kbfocus)
	{
		// Tell game we got focus back, resume music if necessary
		Impl_Unfocused(false);

		if (!firsttimeonmouse && cv_usemouse.value)
			I_StartupMouse();

		if (USE_MOUSEINPUT && !IgnoreMouse())
			SDLdoGrabMouse();
	}
	else if (!mousefocus && !kbfocus)
	{
		// Tell game we lost focus, pause music
		Impl_Unfocused(true);

		if (!disable_mouse)
			SDLforceUngrabMouse();

		if (MOUSE_MENU)
			SDLdoUngrabMouse();
	}
}

static void Impl_HandleKeyboardEvent(SDL_KeyboardEvent evt, Uint32 type)
{
	event_t event;
	if (type == SDL_KEYUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_KEYDOWN)
	{
		event.type = ev_keydown;
	}
	else
	{
		return;
	}
	event.key = Impl_SDL_Scancode_To_Keycode(evt.keysym.scancode);
	if (event.key) D_PostEvent(&event);
}

static void Impl_HandleMouseMotionEvent(SDL_MouseMotionEvent evt)
{
	static boolean firstmove = true;

	if (USE_MOUSEINPUT)
	{
		if ((SDL_GetMouseFocus() != window && SDL_GetKeyboardFocus() != window) || (IgnoreMouse() && !firstmove))
		{
			SDLdoUngrabMouse();
			firstmove = false;
			return;
		}

		// If using relative mouse mode, don't post an event_t just now,
		// add on the offsets so we can make an overall event later.
		if (SDL_GetRelativeMouseMode())
		{
			if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
			{
				mousemovex +=  evt.xrel;
				mousemovey += -evt.yrel;
#if !defined(__ANDROID__)
				SDL_SetWindowGrab(window, SDL_TRUE);
#endif
			}
			firstmove = false;
			return;
		}

		// If the event is from warping the pointer to middle
		// of the screen then ignore it.
		if ((evt.x == realwidth/2) && (evt.y == realheight/2))
		{
			firstmove = false;
			return;
		}

		// Don't send an event_t if not in relative mouse mode anymore,
		// just grab and set relative mode
		// this fixes the stupid camera jerk on mouse entering bug
		// -- Monster Iestyn
		if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
		{
			SDLdoGrabMouse();
		}
	}

	firstmove = false;
}

static void Impl_HandleMouseButtonEvent(SDL_MouseButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	// Ignore the event if the mouse is not actually focused on the window.
	// This can happen if you used the mouse to restore keyboard focus;
	// this apparently makes a mouse button down event but not a mouse button up event,
	// resulting in whatever key was pressed down getting "stuck" if we don't ignore it.
	// -- Monster Iestyn (28/05/18)
	if (SDL_GetMouseFocus() != window || IgnoreMouse())
		return;

	/// \todo inputEvent.button.which
	if (USE_MOUSEINPUT)
	{
		if (type == SDL_MOUSEBUTTONUP)
		{
			event.type = ev_keyup;
		}
		else if (type == SDL_MOUSEBUTTONDOWN)
		{
			event.type = ev_keydown;
		}
		else return;
		if (evt.button == SDL_BUTTON_MIDDLE)
			event.key = KEY_MOUSE1+2;
		else if (evt.button == SDL_BUTTON_RIGHT)
			event.key = KEY_MOUSE1+1;
		else if (evt.button == SDL_BUTTON_LEFT)
			event.key = KEY_MOUSE1;
		else if (evt.button == SDL_BUTTON_X1)
			event.key = KEY_MOUSE1+3;
		else if (evt.button == SDL_BUTTON_X2)
			event.key = KEY_MOUSE1+4;
		if (event.type == ev_keyup || event.type == ev_keydown)
		{
			D_PostEvent(&event);
		}
	}
}

static void Impl_HandleMouseWheelEvent(SDL_MouseWheelEvent evt)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	if (evt.y > 0)
	{
		event.key = KEY_MOUSEWHEELUP;
		event.type = ev_keydown;
	}
	if (evt.y < 0)
	{
		event.key = KEY_MOUSEWHEELDOWN;
		event.type = ev_keydown;
	}
	if (evt.y == 0)
	{
		event.key = 0;
		event.type = ev_keyup;
	}
	if (event.type == ev_keyup || event.type == ev_keydown)
	{
		D_PostEvent(&event);
	}
}

static INT32 SDLJoyAxis(const Sint16 axis, evtype_t which)
{
	// -32768 to 32767
	INT32 raxis = axis/32;
	JoyType_t *Joystick_p = (which == ev_joystick2) ? &Joystick2 : &Joystick;
	SDLJoyInfo_t *JoyInfo_p = (which == ev_joystick2) ? &JoyInfo2 : &JoyInfo;

	if (Joystick_p->bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (raxis < -(JOYAXISRANGE/2))
			raxis = -1;
		else if (raxis > (JOYAXISRANGE/2))
			raxis = 1;
		else
			raxis = 0;
	}
	else
	{
		raxis = JoyInfo_p->scale!=1?((raxis/JoyInfo_p->scale)*JoyInfo_p->scale):raxis;

#ifdef SDL_JDEADZONE
		if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
			raxis = 0;
#endif
	}

	return raxis;
}

static INT32 AccelerometerAxis(const INT32 axis)
{
#ifdef ACCELEROMETER
	return (axis / 32) * cv_accelscale.value;
#else
	(void)axis;
	return 0;
#endif
}

static INT32 AccelerometerTilt(void)
{
#ifdef ACCELEROMETER
	fixed_t tilt = cv_acceltilt.value;

	tilt = FixedDiv(tilt, ACCELEROMETER_MAX_TILT_OFFSET);
	tilt = FixedMul(tilt, 32768*FRACUNIT);

	return FixedInt(tilt);
#else
	return 0;
#endif
}

static void Impl_HandleJoystickAxisEvent(SDL_JoyAxisEvent evt)
{
	event_t event;

	evt.axis++;
	if (evt.axis > JOYAXISSET*2)
		return;

	event.key = event.x = event.y = INT32_MAX;

	if (evt.which == SDL_JoystickInstanceID(AccelerometerDevice))
	{
		if (G_CanUseAccelerometer())
			event.type = ev_accelerometer;
		else
			return;

		if (evt.axis == 3) // Move forwards / backwards
			event.y = AccelerometerAxis((INT32)(-evt.value) - AccelerometerTilt());
		else if (evt.axis == 1) // Move sideways
			event.x = AccelerometerAxis(evt.value);
		else
			return;
	}
	else
	{
		SDL_JoystickID joyid[2];

		// Determine the Joystick IDs for each current open joystick
		joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
		joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

		if (evt.which == joyid[0])
			event.type = ev_joystick;
		else if (evt.which == joyid[1])
			event.type = ev_joystick2;
		else
			return;

		if (evt.axis%2)
		{
			event.key = evt.axis / 2;
			event.x = SDLJoyAxis(evt.value, event.type);
		}
		else
		{
			evt.axis--;
			event.key = evt.axis / 2;
			event.y = SDLJoyAxis(evt.value, event.type);
		}
	}

	D_PostEvent(&event);
}

#if 0
static void Impl_HandleJoystickHatEvent(SDL_JoyHatEvent evt)
{
	event_t event;
	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.hat >= JOYHATS)
		return; // ignore hats with too high an index

	if (evt.which == joyid[0])
	{
		event.key = KEY_HAT1 + (evt.hat*4);
	}
	else if (evt.which == joyid[1])
	{
		event.key = KEY_2HAT1 + (evt.hat*4);
	}
	else return;

	// NOTE: UNFINISHED
}
#endif

static void Impl_HandleJoystickButtonEvent(SDL_JoyButtonEvent evt, Uint32 type)
{
	event_t event;

	if (I_JoystickIsTVRemote(evt.which + 1))
	{
		switch (evt.button)
		{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				event.key = KEY_REMOTEUP;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				event.key = KEY_REMOTEDOWN;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				event.key = KEY_REMOTELEFT;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				event.key = KEY_REMOTERIGHT;
				break;
			case SDL_CONTROLLER_BUTTON_A:
				event.key = KEY_REMOTECENTER;
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				event.key = KEY_REMOTEBACK;
				break;
			default:
				return;
		}
	}
	else
	{
		SDL_JoystickID joyid[2];

		// Determine the Joystick IDs for each current open joystick
		joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
		joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

		if (evt.which == joyid[0])
		{
			event.key = KEY_JOY1;
		}
		else if (evt.which == joyid[1])
		{
			event.key = KEY_2JOY1;
		}
		else return;

		if (evt.button < JOYBUTTONS)
		{
			event.key += evt.button;
		}
		else return;
	}

	if (type == SDL_JOYBUTTONUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_JOYBUTTONDOWN)
	{
		event.type = ev_keydown;
	}
	else return;

	D_PostEvent(&event);
}

#ifdef TOUCHINPUTS
static void Impl_GetDrawableSize(int *w, int *h)
{
	*w = realwidth;
	*h = realheight;
}

static float Impl_GetDPI(void)
{
	return 1.0f;
}

static void Impl_DenormalizeTouchCoords(float *x, float *y)
{
	int w, h;
	int pw, ph;
	float scale;

	SDL_GetWindowSize(window, &w, &h);
	Impl_GetDrawableSize(&pw, &ph);

	// Transform to window coordinates
	*x = (*x) * ((float)pw / (float)w);
	*y = (*y) * ((float)ph / (float)h);

	// Divide by DPI
	scale = Impl_GetDPI();
	*x = (*x) / scale;
	*y = (*y) / scale;

	// Multiply by the window's size
	SDL_GetWindowSize(window, &w, &h);
	*x = (*x) * (float)w;
	*y = (*y) * (float)h;
}

static void Impl_HandleTouchEvent(SDL_TouchFingerEvent evt)
{
	event_t event;
	touchevent_t finger;
	INT32 id = (INT32)(evt.fingerId);
	float x, y, dx, dy;

	if (id >= NUMTOUCHFINGERS)
		return;

	switch (evt.type)
	{
		case SDL_FINGERMOTION:
			event.type = ev_touchmotion;
			break;
		case SDL_FINGERDOWN:
			event.type = ev_touchdown;
			break;
		case SDL_FINGERUP:
			event.type = ev_touchup;
			break;
		default:
			// Don't generate any event.
			return;
	}

	finger.fx = evt.x;
	finger.fy = evt.y;
	finger.fdx = (float)(evt.dx);
	finger.fdy = (float)(-evt.dy);

	Impl_DenormalizeTouchCoords(&finger.fx, &finger.fy);
	x = roundf(finger.fx);
	y = roundf(finger.fy);

	Impl_DenormalizeTouchCoords(&finger.fdx, &finger.fdy);
	dx = roundf(finger.fdx);
	dy = roundf(finger.fdy);

	finger.x = (INT32)x;
	finger.y = (INT32)y;
	finger.dx = (INT32)dx;
	finger.dy = (INT32)dy;
	finger.pressure = evt.pressure;

	// Acknowledges the current finger's state.
	TS_OnTouchEvent(id, event.type, &finger);

	// Push an event into the responder queue.
	// The key (finger id) will be used to retrieve the touch event's information.
	event.key = id;
	event.x = finger.x;
	event.y = finger.y;

	D_PostEvent(&event);

	// A touch screen is now recognized as present in the device.
	if (!touchscreenavailable)
	{
		touchscreenavailable = true;
		I_TouchScreenAvailable();
	}
}

// On-screen keyboard text input
static char *textinputbuffer = NULL;
static size_t textbufferlength = 0;
static void (*textinputcallback)(char *, size_t);

static void Impl_HandleTextInput(SDL_TextInputEvent evt)
{
	char *text;
	size_t length, i;

	text = evt.text;
	length = strlen(text);

	if (textinputcallback != NULL)
	{
		textinputcallback(text, length);
		return;
	}

	if (textinputbuffer == NULL)
		return;

	if (strlen(textinputbuffer) >= textbufferlength)
		return;

	for (i = 0; i < length; i++)
	{
		char thischar = text[i];
		if (((unsigned)thischar) >= 32 && ((unsigned)thischar) <= 127)
		{
			char cat[2];
			cat[0] = thischar;
			cat[1] = '\0';
			strcat(textinputbuffer, cat);
		}
		if (strlen(textinputbuffer) >= textbufferlength)
			break;
	}
}
#endif

void I_GetEvent(void)
{
	SDL_Event evt;
	// We only want the first motion event,
	// otherwise we'll end up catching the warp back to center.
	//int mouseMotionOnce = 0;

	if (!graphics_started)
	{
		return;
	}

	mousemovex = mousemovey = 0;

	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			case SDL_WINDOWEVENT:
				Impl_HandleWindowEvent(evt.window);
				break;
#if defined(__ANDROID__)
			case SDL_APP_WILLENTERBACKGROUND:
				appOnBackground = SDL_TRUE;
				Impl_Unfocused(true);
				break;
			case SDL_APP_WILLENTERFOREGROUND:
				appOnBackground = SDL_FALSE;
				Impl_AppEnteredForeground();
				Impl_Unfocused(false);
				break;
#endif
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				Impl_HandleKeyboardEvent(evt.key, evt.type);
				break;
			case SDL_MOUSEMOTION:
				//if (!mouseMotionOnce)
				Impl_HandleMouseMotionEvent(evt.motion);
				//mouseMotionOnce = 1;
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				Impl_HandleMouseButtonEvent(evt.button, evt.type);
				break;
			case SDL_MOUSEWHEEL:
				Impl_HandleMouseWheelEvent(evt.wheel);
				break;
			case SDL_JOYAXISMOTION:
				Impl_HandleJoystickAxisEvent(evt.jaxis);
				break;
#ifdef TOUCHINPUTS
			case SDL_FINGERMOTION:
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
				Impl_HandleTouchEvent(evt.tfinger);
				break;
			case SDL_TEXTINPUT:
				// If user pressed the console button, don't type the
				// character into the console buffer.
				if (evt.text.text[0] && !evt.text.text[1]
					&& evt.text.text[0] != gamecontrol[gc_console][0]
					&& evt.text.text[0] != gamecontrol[gc_console][1])
					Impl_HandleTextInput(evt.text);
				break;
#endif
#if 0
			case SDL_JOYHATMOTION:
				Impl_HandleJoystickHatEvent(evt.jhat)
				break;
#endif
			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				Impl_HandleJoystickButtonEvent(evt.jbutton, evt.type);
				break;
			case SDL_JOYDEVICEADDED:
				{
					INT32 index = evt.jdevice.which;
					SDL_Joystick *newjoy = SDL_JoystickOpen(index);

					CONS_Debug(DBG_GAMELOGIC, "Joystick device index %d added\n", index + 1);

					// Turns out I shouldn't care.
					if (I_JoystickIsTVRemote(index + 1))
					{
						if (TVRemoteDevice)
							SDL_JoystickClose(TVRemoteDevice);
						TVRemoteDevice = newjoy;
						break;
					}
					else if (I_JoystickIsAccelerometer(index + 1))
					{
						if (AccelerometerDevice)
							SDL_JoystickClose(AccelerometerDevice);
						AccelerometerDevice = newjoy;
						break;
					}

					// Because SDL's device index is unstable, we're going to cheat here a bit:
					// For the first joystick setting that is NOT active:
					// 1. Set cv_usejoystickX.value to the new device index (this does not change what is written to config.cfg)
					// 2. Set OTHERS' cv_usejoystickX.value to THEIR new device index, because it likely changed
					//    * If device doesn't exist, switch cv_usejoystick back to default value (.string)
					//      * BUT: If that default index is being occupied, use ANOTHER cv_usejoystick's default value!
					if (newjoy && (!JoyInfo.dev || !SDL_JoystickGetAttached(JoyInfo.dev))
						&& JoyInfo2.dev != newjoy) // don't override a currently active device
					{
						cv_usejoystick.value = index + 1;

						if (JoyInfo2.dev)
							cv_usejoystick2.value = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
						else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick2.string);
						else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick.string);
						else // we tried...
							cv_usejoystick2.value = 0;
					}
					else if (newjoy && (!JoyInfo2.dev || !SDL_JoystickGetAttached(JoyInfo2.dev))
						&& JoyInfo.dev != newjoy) // don't override a currently active device
					{
						cv_usejoystick2.value = index + 1;

						if (JoyInfo.dev)
							cv_usejoystick.value = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
						else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick.string);
						else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick2.string);
						else // we tried...
							cv_usejoystick.value = 0;
					}

					// Was cv_usejoystick disabled in settings?
					if (!strcmp(cv_usejoystick.string, "0") || !cv_usejoystick.value)
						cv_usejoystick.value = 0;
					else if (atoi(cv_usejoystick.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						     && cv_usejoystick.value) // update the cvar ONLY if a device exists
						CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

					if (!strcmp(cv_usejoystick2.string, "0") || !cv_usejoystick2.value)
						cv_usejoystick2.value = 0;
					else if (atoi(cv_usejoystick2.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
					         && cv_usejoystick2.value) // update the cvar ONLY if a device exists
						CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

					// Update all joysticks' init states
					// This is a little wasteful since cv_usejoystick already calls this, but
					// we need to do this in case CV_SetValue did nothing because the string was already same.
					// if the device is already active, this should do nothing, effectively.
					I_ChangeJoystick();
					I_ChangeJoystick2();

					CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);

					// update the menu
					if (currentMenu == &OP_JoystickSetDef)
						M_SetupJoystickMenu(0);

					if (JoyInfo.dev != newjoy && JoyInfo2.dev != newjoy)
						SDL_JoystickClose(newjoy);
				}
				break;
			case SDL_JOYDEVICEREMOVED:
				// TV remotes can (and will) disconnect. Just close the device.
				// Accelerometers will never disconnect. Unless I'm mistaken.
				if (TVRemoteDevice && evt.jdevice.which == SDL_JoystickInstanceID(TVRemoteDevice))
				{
					SDL_JoystickClose(TVRemoteDevice);
					TVRemoteDevice = NULL;
					break;
				}

				if (JoyInfo.dev && !SDL_JoystickGetAttached(JoyInfo.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick1 removed, device index: %d\n", JoyInfo.oldjoy);
					I_ShutdownJoystick();
				}

				if (JoyInfo2.dev && !SDL_JoystickGetAttached(JoyInfo2.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 removed, device index: %d\n", JoyInfo2.oldjoy);
					I_ShutdownJoystick2();
				}

				// Update the device indexes, because they likely changed
				// * If device doesn't exist, switch cv_usejoystick back to default value (.string)
				//   * BUT: If that default index is being occupied, use ANOTHER cv_usejoystick's default value!
				if (JoyInfo.dev)
					cv_usejoystick.value = JoyInfo.oldjoy = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
				else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick.string);
				else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick2.string);
				else // we tried...
					cv_usejoystick.value = 0;

				if (JoyInfo2.dev)
					cv_usejoystick2.value = JoyInfo2.oldjoy = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
				else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick2.string);
				else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick.string);
				else // we tried...
					cv_usejoystick2.value = 0;

				// Was cv_usejoystick disabled in settings?
				if (!strcmp(cv_usejoystick.string, "0"))
					cv_usejoystick.value = 0;
				else if (atoi(cv_usejoystick.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						 && cv_usejoystick.value) // update the cvar ONLY if a device exists
					CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

				if (!strcmp(cv_usejoystick2.string, "0"))
					cv_usejoystick2.value = 0;
				else if (atoi(cv_usejoystick2.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						 && cv_usejoystick2.value) // update the cvar ONLY if a device exists
					CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

				CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
				CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);

				// update the menu
				if (currentMenu == &OP_JoystickSetDef)
					M_SetupJoystickMenu(0);
			 	break;
			case SDL_QUIT:
				LUAh_GameQuit(true);
				I_Quit();
				break;
		}
	}

	// Send all relative mouse movement as one single mouse event.
	if (mousemovex || mousemovey)
	{
		event_t event;
		int wwidth, wheight;
		SDL_GetWindowSize(window, &wwidth, &wheight);
		//SDL_memset(&event, 0, sizeof(event_t));
		event.type = ev_mouse;
		event.key = 0;
		event.x = (INT32)lround(mousemovex * ((float)wwidth / (float)realwidth));
		event.y = (INT32)lround(mousemovey * ((float)wheight / (float)realheight));
		D_PostEvent(&event);
	}

	// In order to make wheels act like buttons, we have to set their state to Up.
	// This is because wheel messages don't have an up/down state.
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;

	if (disable_mouse)
		return;

	if (!firsttimeonmouse)
	{
		HalfWarpMouse(realwidth, realheight); // warp to center
	}
	else
		firsttimeonmouse = SDL_FALSE;
	if (cv_usemouse.value && !IgnoreMouse())
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}

#ifdef TOUCHINPUTS
void I_RaiseScreenKeyboard(char *buffer, size_t length)
{
	textinputbuffer = buffer;
	textbufferlength = length;
	textinputcallback = NULL;
	SDL_StartTextInput();
}

void I_ScreenKeyboardCallback(void (*callback)(char *, size_t))
{
	textinputcallback = callback;
}

boolean I_KeyboardOnScreen(void)
{
	return ((SDL_IsTextInputActive() == SDL_TRUE) ? true : false);
}

void I_CloseScreenKeyboard(void)
{
	textinputbuffer = NULL;
	textbufferlength = 0;
	textinputcallback = NULL;
	SDL_StopTextInput();
}
#endif

//
// I_OsPolling
//
void I_OsPolling(void)
{
	SDL_Keymod mod;

	if (consolevent)
		I_GetConsoleEvents();
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		SDL_JoystickUpdate();
		I_GetJoystickEvents();
		I_GetJoystick2Events();
	}

	I_GetMouseEvents();

	I_GetEvent();

	mod = SDL_GetModState();
	/* Handle here so that our state is always synched with the system. */
	shiftdown = ctrldown = altdown = 0;
	capslock = false;
	if (mod & KMOD_LSHIFT) shiftdown |= 1;
	if (mod & KMOD_RSHIFT) shiftdown |= 2;
	if (mod & KMOD_LCTRL)   ctrldown |= 1;
	if (mod & KMOD_RCTRL)   ctrldown |= 2;
	if (mod & KMOD_LALT)     altdown |= 1;
	if (mod & KMOD_RALT)     altdown |= 2;
	if (mod & KMOD_CAPS) capslock = true;
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{

}

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame
// from PrBoom's src/SDL/i_video.c
static inline boolean I_SkipFrame(void)
{
#if 0
	static boolean skip = false;

	if (rendermode != render_soft)
		return false;

	skip = !skip;

	switch (gamestate)
	{
		case GS_LEVEL:
			if (!paused)
				return false;
			/* FALLTHRU */
		//case GS_TIMEATTACK: -- sorry optimisation but now we have a cool level platter and that being laggardly looks terrible
		case GS_WAITINGPLAYERS:
			return skip; // Skip odd frames
		default:
			return false;
	}
#endif
	return false;
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
	if (rendermode == render_none)
		return; //Alam: No software or OpenGl surface

	if (I_SkipFrame())
		return;

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (cv_showping.value && netgame && consoleplayer != serverplayer)
		SCR_DisplayLocalPing();

#ifdef TOUCHINPUTS
	if (touchscreenavailable && cv_showfingers.value && !(takescreenshot && !cv_touchscreenshots.value))
		SCR_DisplayFingers();
#endif

	if (appOnBackground == SDL_TRUE)
		return;

	if (rendermode == render_soft && screens[0])
	{
		if (!bufSurface) //Double-Check
		{
			Impl_VideoSetupSDLBuffer();
		}
		if (bufSurface)
		{
			VID_BlitSurfaceRegion(0, 0, vid.width, vid.height);
		}
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl)
	{
		OglSdlFinishUpdate(cv_vidwait.value);
	}
#endif
}

//
// I_UpdateNoVsync
//
void I_UpdateNoVsync(void)
{
	INT32 real_vidwait = cv_vidwait.value;
	cv_vidwait.value = 0;
	I_FinishUpdate();
	cv_vidwait.value = real_vidwait;
}

//
// I_ReadScreen
//
void I_ReadScreen(UINT8 *scr)
{
	if (rendermode != render_soft)
		I_Error ("I_ReadScreen: called while in non-software mode");
	else
		VID_BlitLinearScreen(screens[0], scr,
			vid.width*vid.bpp, vid.height,
			vid.rowbytes, vid.rowbytes);
}

//
// I_SetPalette
//
void I_SetPalette(RGBA_t *palette)
{
	size_t i;
	for (i=0; i<256; i++)
	{
		localPalette[i].r = palette[i].s.red;
		localPalette[i].g = palette[i].s.green;
		localPalette[i].b = palette[i].s.blue;
	}
	//if (vidSurface) SDL_SetPaletteColors(vidSurface->format->palette, localPalette, 0, 256);
	// Fury -- SDL2 vidSurface is a 32-bit surface buffer copied to the texture. It's not palletized, like bufSurface.
	if (bufSurface) SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
}

// return number of fullscreen + X11 modes
INT32 VID_NumModes(void)
{
	return MAXWINMODES;
}

const char *VID_GetModeName(INT32 modeNum)
{
	if (modeNum == -1)
		return fallback_resolution_name;
	else if (modeNum > MAXWINMODES)
		return NULL;

	snprintf(&vidModeName[modeNum][0], 32, "%dx%d", windowedModes[modeNum][0], windowedModes[modeNum][1]);

	return &vidModeName[modeNum][0];
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	int i;
	for (i = 0; i < MAXWINMODES; i++)
	{
		if (windowedModes[i][0] == w && windowedModes[i][1] == h)
		{
			return i;
		}
	}
	return -1;
}

void VID_PrepareModeList(void)
{
	// Under SDL2, we just use the windowed modes list, and scale in windowed fullscreen.
	allow_fullscreen = true;
}

void VID_DisplayGLError(void)
{
#ifdef HWRENDER
	CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");

	if (menuactive)
	{
		if (lastglerror)
			M_StartMessage(va(M_GetText("OpenGL failed to load:\n%s\n\n%s"), lastglerror, M_GetUserActionString(PRESS_A_KEY_MESSAGE)), NULL, MM_NOTHING);
		else
			M_ShowAnyKeyMessage("OpenGL failed to load.\nCheck the console\nor log file for details.\n\n");
	}
#endif
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
#ifdef HWRENDER
	if (vid.glstate == VID_GL_LIBRARY_ERROR) // Well, it didn't work the first time anyway.
	{
		renderswitcherror = render_opengl;
		rendermode = oldrender;
		if (chosenrendermode == render_opengl) // fallback to software
			rendermode = render_soft;
		if (setrenderneeded)
		{
			CV_StealthSetValue(&cv_renderer, oldrender);
			setrenderneeded = 0;
		}
	}
#endif
}

INT32 VID_CheckRenderer(void)
{
	INT32 rendererchanged = 0;
	boolean contextcreated = false;
#ifdef HWRENDER
	rendermode_t oldrenderer = rendermode;
#endif

	if (dedicated)
		return 0;

	if (setrenderneeded)
	{
		rendermode = setrenderneeded;
		rendererchanged = 1;

#if defined(__ANDROID__)
		SDL_RenderSetLogicalSize(renderer, vid.width, vid.height);
#endif

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			VID_CheckGLLoaded(oldrenderer);

			// Initialise OpenGL before calling SDLSetMode!!!
			// This is because SDLSetMode calls OglSdlSurface.
			if (vid.glstate == VID_GL_LIBRARY_NOTLOADED)
			{
				VID_StartupOpenGL();

#if !defined(__ANDROID__)
				// Loaded successfully!
				if (vid.glstate == VID_GL_LIBRARY_LOADED)
				{
					// Destroy the current window, if it exists.
					if (window)
					{
						SDL_DestroyWindow(window);
						window = NULL;
					}

					// Destroy the current window rendering context, if that also exists.
					if (renderer)
						VID_DestroyContext();

					// Create a new window.
					Impl_CreateWindow(USE_FULLSCREEN);

					// From there, the OpenGL context was already created.
					contextcreated = true;
				}
#endif
			}
			else if (vid.glstate == VID_GL_LIBRARY_ERROR)
			{
				renderswitcherror = render_opengl;
				rendererchanged = 0;
			}
		}
#endif

		if (!contextcreated)
			VID_CreateContext();

		setrenderneeded = 0;
	}

	SDLSetMode(vid.width, vid.height, USE_FULLSCREEN, (setmodeneeded ? SDL_TRUE : SDL_FALSE));
	Impl_VideoSetupBuffer();

	if (rendermode == render_soft)
	{
		if (bufSurface)
		{
			SDL_FreeSurface(bufSurface);
			bufSurface = NULL;
		}

		SCR_SetDrawFuncs();
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && rendererchanged)
	{
		HWR_Switch();
		V_SetPalette(0);
	}
#endif

#ifdef DITHER
	if (rendererchanged)
		Impl_SetDither();
#endif

	return rendererchanged;
}

void VID_GetNativeResolution(INT32 *width, INT32 *height)
{
	SDL_DisplayMode resolution;
	int i;

	for (i = 0; i < SDL_GetNumVideoDisplays(); i++)
	{
		if (!SDL_GetCurrentDisplayMode(i, &resolution))
		{
			if (width)
				*width = (INT32)(resolution.w);
			if (height)
				*height = (INT32)(resolution.h);
			return;
		}
	}
}

INT32 VID_SetMode(INT32 modeNum)
{
	SDLdoUngrabMouse();

	vid.recalc = 1;
	vid.bpp = 1;

#ifdef NATIVESCREENRES
	if (cv_nativeres.value)
	{
		INT32 w = 0, h = 0;

		SCR_GetNativeResolution(&w, &h);

		vid.width = (INT32)((float)w / scr_resdiv);
		vid.height = (INT32)((float)h / scr_resdiv);

		if (vid.width > MAXVIDWIDTH)
			vid.width = MAXVIDWIDTH;
		else if (vid.width < BASEVIDWIDTH)
			vid.width = BASEVIDWIDTH;

		if (vid.height > MAXVIDHEIGHT)
			vid.height = MAXVIDHEIGHT;
		else if (vid.height < BASEVIDHEIGHT)
			vid.height = BASEVIDHEIGHT;

		vid.modenum = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value);
	}
	else
#endif
	{
		if (modeNum < 0)
			modeNum = 0;
		if (modeNum >= MAXWINMODES)
			modeNum = MAXWINMODES-1;

		vid.width = windowedModes[modeNum][0];
		vid.height = windowedModes[modeNum][1];
		vid.modenum = modeNum;
	}

	VID_CheckRenderer();
	return SDL_TRUE;
}

INT32 VID_CreateContext(void)
{
	// Renderer-specific stuff
#ifdef HWRENDER
	if ((rendermode == render_opengl)
	&& (vid.glstate != VID_GL_LIBRARY_ERROR))
	{
		if (!sdlglcontext)
			sdlglcontext = SDL_GL_CreateContext(window);
		if (sdlglcontext == NULL)
		{
			SDL_DestroyWindow(window);
			I_Error("Failed to create a GL context: %s\n", SDL_GetError());
		}
		SDL_GL_MakeCurrent(window, sdlglcontext);
	}
	else
#endif
	if (rendermode == render_soft)
	{
		int flags = 0; // Use this to set SDL_RENDERER_* flags now
		if (usesdl2soft)
			flags |= SDL_RENDERER_SOFTWARE;
		else if (cv_vidwait.value)
			flags |= SDL_RENDERER_PRESENTVSYNC;

		if (!renderer)
			renderer = SDL_CreateRenderer(window, -1, flags);
		if (renderer == NULL)
		{
			CONS_Printf(M_GetText("Couldn't create rendering context: %s\n"), SDL_GetError());
			return 0;
		}
		SDL_RenderSetLogicalSize(renderer, BASEVIDWIDTH, BASEVIDHEIGHT);
	}

#ifdef DITHER
	Impl_SetDither();
#endif

	return 1;
}

INT32 VID_DestroyContext(void)
{
	if (!renderer)
		return 0;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	return 1;
}

void VID_BlitSurfaceRegion(INT32 x, INT32 y, INT32 w, INT32 h)
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_BlitSurface(bufSurface, NULL, vidSurface, &rect);
	// Fury -- there's no way around UpdateTexture, the GL backend uses it anyway
	SDL_LockSurface(vidSurface);
	SDL_UpdateTexture(texture, &rect, vidSurface->pixels, vidSurface->pitch);
	SDL_UnlockSurface(vidSurface);
}

void VID_ClearTexture(void)
{
	if (renderer)
		SDL_RenderClear(renderer);
}

void VID_PresentTexture(void)
{
	if (renderer)
	{
		if (texture)
			SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
}

static SDL_bool Impl_CreateWindow(SDL_bool fullscreen)
{
	int flags = 0;

	if (rendermode == render_none) // dedicated
		return SDL_TRUE; // Monster Iestyn -- not sure if it really matters what we return here tbh

	if (window != NULL)
		return SDL_FALSE;

	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (borderlesswindow)
		flags |= SDL_WINDOW_BORDERLESS;

#ifdef HWRENDER
#if !defined(__ANDROID__)
	if (vid.glstate == VID_GL_LIBRARY_LOADED)
#endif
		flags |= SDL_WINDOW_OPENGL;
#endif

#if defined(__ANDROID__)
	Impl_SetColorBufferDepth(8, 8, 8, 8);
#endif

	// Create a window
	window = SDL_CreateWindow("SRB2 "VERSIONSTRING, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			realwidth, realheight, flags);

	if (window == NULL)
	{
		CONS_Printf(M_GetText("Couldn't create window: %s\n"), SDL_GetError());
		return SDL_FALSE;
	}

	Impl_SetWindowIcon();

	if (!VID_CreateContext())
		return SDL_FALSE;

	return SDL_TRUE;
}

static void Impl_SetWindowIcon(void)
{
	if (window && icoSurface)
		SDL_SetWindowIcon(window, icoSurface);
}

static void Impl_VideoSetupSDLBuffer(void)
{
	if (bufSurface != NULL)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}
	// Set up the SDL palletized buffer (copied to vidbuffer before being rendered to texture)
	if (vid.bpp == 1)
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,8,
			(int)vid.rowbytes,0x00000000,0x00000000,0x00000000,0x00000000); // 256 mode
	}
	else if (vid.bpp == 2) // Fury -- don't think this is used at all anymore
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,15,
			(int)vid.rowbytes,0x00007C00,0x000003E0,0x0000001F,0x00000000); // 555 mode
	}
	if (bufSurface)
	{
		SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
	}
	else
	{
		I_Error("%s", M_GetText("No system memory for SDL buffer surface\n"));
	}
}

static void Impl_VideoSetupBuffer(void)
{
	// Set up game's software render buffer
	vid.rowbytes = vid.width * vid.bpp;
	vid.direct = NULL;
	if (vid.buffer)
		free(vid.buffer);
	vid.buffer = calloc(vid.rowbytes*vid.height, NUMSCREENS);
	if (!vid.buffer)
	{
		I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
	}
}

#ifdef HAVE_GLES
static void Impl_InitGLESDriver(void)
{
	const char *driver_name = NULL;
	int version_major, version_minor;

#ifdef HAVE_GLES2
	driver_name = "opengles2";
	version_major = 2;
	version_minor = 0;
#else
	driver_name = "opengles";
	version_major = 1;
	version_minor = 1;
#endif

	SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver_name);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version_major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version_minor);
}
#endif

#if defined(__ANDROID__)
static void Impl_SetColorBufferDepth(INT32 red, INT32 green, INT32 blue, INT32 alpha)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, red);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, green);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blue);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha);
}
#endif

static void Impl_InitVideoSubSystem(void)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		CONS_Printf(M_GetText("Couldn't initialize SDL's Video System: %s\n"), SDL_GetError());
		return;
	}

#if defined(__ANDROID__)
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
#ifdef HAVE_GLES
	Impl_InitGLESDriver();
#endif
}

#ifdef DITHER
static void Impl_SetDither(void)
{
	if (!graphics_started)
		return;

	if (rendermode == render_soft)
	{
#if defined(__ANDROID__)
		if (cv_dither.value)
			glEnable(GL_DITHER);
		else
			glDisable(GL_DITHER);
#endif
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
		HWD.pfnSetSpecialState(HWD_SET_DITHER, cv_dither.value);
#endif
}
#endif

void I_StartupGraphics(void)
{
	if (dedicated)
	{
		rendermode = render_none;
		return;
	}
	if (graphics_started)
		return;

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand ("vid_info", VID_Command_Info_f);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_stretch);
#ifdef DITHER
	CV_RegisterVar (&cv_dither);
#endif
	CV_RegisterVar (&cv_alwaysgrabmouse);
	disable_mouse = M_CheckParm("-nomouse");
	disable_fullscreen = M_CheckParm("-win") ? 1 : 0;

	keyboard_started = true;

#if !defined(HAVE_TTF) && !defined(SPLASH_SCREEN)
	// Previously audio was init here for questionable reasons?
	Impl_InitVideoSubSystem();
#endif
	{
		const char *vd = SDL_GetCurrentVideoDriver();
		//CONS_Printf(M_GetText("Starting up with video driver: %s\n"), vd);
		if (vd && (
			strncasecmp(vd, "gcvideo", 8) == 0 ||
			strncasecmp(vd, "fbcon", 6) == 0 ||
			strncasecmp(vd, "wii", 4) == 0 ||
			strncasecmp(vd, "psl1ght", 8) == 0
		))
			framebuffer = SDL_TRUE;
	}

#ifdef SPLASH_SCREEN_SUPPORTED
	// free splash screen image data
	SplashScreen_FreeImage();
#endif

	// free last video surface
	if (vidSurface)
	{
		SDL_FreeSurface(vidSurface);
		vidSurface = NULL;
	}

	// free last buffer surface
	if (bufSurface)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	// Renderer choices
	// Takes priority over the config.
	if (M_CheckParm("-renderer"))
	{
		INT32 i = 0;
		CV_PossibleValue_t *renderer_list = cv_renderer_t;
		const char *modeparm = M_GetNextParm();
		while (renderer_list[i].strvalue)
		{
			if (!stricmp(modeparm, renderer_list[i].strvalue))
			{
				chosenrendermode = renderer_list[i].value;
				break;
			}
			i++;
		}
	}

	// Choose Software renderer
	else if (M_CheckParm("-software"))
		chosenrendermode = render_soft;

#ifdef HWRENDER
	// Choose OpenGL renderer
	else if (M_CheckParm("-opengl"))
		chosenrendermode = render_opengl;

	// Don't startup OpenGL
	if (M_CheckParm("-nogl"))
	{
		vid.glstate = VID_GL_LIBRARY_ERROR;
		if (chosenrendermode == render_opengl)
			chosenrendermode = render_none;
	}
#endif

	if (chosenrendermode != render_none)
		rendermode = chosenrendermode;

	usesdl2soft = M_CheckParm("-softblit");
	borderlesswindow = M_CheckParm("-borderless");

	// finish splash screen
	if (splashScreen.displaying == SDL_TRUE)
	{
		splashScreen.displaying = SDL_FALSE;

#if defined(HWRENDER) && !defined(__ANDROID__)
		// Destroy the window and the renderer
		if (rendermode == render_opengl)
		{
			SDL_DestroyWindow(window);
			VID_DestroyContext();
			window = NULL;
		}
#endif
	}

	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY>>1,SDL_DEFAULT_REPEAT_INTERVAL<<2);
	VID_Command_ModeList_f();

#ifdef HWRENDER
	if (rendermode == render_opengl)
		VID_StartupOpenGL();
#endif

	// Window icon
#ifdef HAVE_IMAGE
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif

	// Fury: we do window initialization after GL setup to allow
	// SDL_GL_LoadLibrary to work well on Windows

	// Create window
	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	vid.width = BASEVIDWIDTH; // Default size for startup
	vid.height = BASEVIDHEIGHT; // BitsPerPixel is the SDL interface's
	vid.recalc = true; // Set up the console stufff
	vid.direct = NULL; // Maybe direct access?
	vid.bpp = 1; // This is the game engine's Bpp
	vid.WndParent = NULL; //For the window?

#ifdef HAVE_TTF
	I_ShutdownTTF();
#endif

#if !defined(__ANDROID__)
	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));
#endif

	if (M_CheckParm("-nomousegrab"))
		mousegrabok = SDL_FALSE;
#if 0 // defined (_DEBUG)
	else
	{
		char videodriver[4] = {'S','D','L',0};
		if (!M_CheckParm("-mousegrab") &&
		    *strncpy(videodriver, SDL_GetCurrentVideoDriver(), 4) != '\0' &&
		    strncasecmp("x11",videodriver,4) == 0)
			mousegrabok = SDL_FALSE; //X11's XGrabPointer not good
	}
#endif
	realwidth = (Uint16)vid.width;
	realheight = (Uint16)vid.height;

	VID_Command_Info_f();
	SDLdoUngrabMouse();

	SDL_RaiseWindow(window);

	if (mousegrabok && !disable_mouse)
		SDLdoGrabMouse();

	graphics_started = true;
}

void VID_StartupOpenGL(void)
{
#ifdef HWRENDER
	static boolean glstartup = false;
	if (!glstartup)
	{
		CONS_Printf("VID_StartupOpenGL()...\n");
		HWD.pfnInit             = hwSym("Init",NULL);
		HWD.pfnFinishUpdate     = NULL;
		HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
		HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
		HWD.pfnDrawPolygonShader = hwSym("DrawPolygonShader",NULL);
		HWD.pfnDrawIndexedTriangles = hwSym("DrawIndexedTriangles",NULL);
		HWD.pfnRenderSkyDome    = hwSym("RenderSkyDome",NULL);
		HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
		HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
		HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
		HWD.pfnUpdateTexture    = hwSym("UpdateTexture",NULL);
		HWD.pfnDeleteTexture    = hwSym("DeleteTexture",NULL);
		HWD.pfnReadRect         = hwSym("ReadRect",NULL);
		HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
		HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
		HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
		HWD.pfnSetPalette       = hwSym("SetPalette",NULL);
		HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
		HWD.pfnDrawModel        = hwSym("DrawModel",NULL);
		HWD.pfnCreateModelVBOs  = hwSym("CreateModelVBOs",NULL);
		HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
		HWD.pfnPostImgRedraw    = hwSym("PostImgRedraw",NULL);
		HWD.pfnFlushScreenTextures=hwSym("FlushScreenTextures",NULL);
		HWD.pfnStartScreenWipe  = hwSym("StartScreenWipe",NULL);
		HWD.pfnEndScreenWipe    = hwSym("EndScreenWipe",NULL);
		HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
		HWD.pfnDoTintedWipe     = hwSym("DoTintedWipe",NULL);
		HWD.pfnDrawIntermissionBG=hwSym("DrawIntermissionBG",NULL);
		HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
		HWD.pfnMakeScreenFinalTexture=hwSym("MakeScreenFinalTexture",NULL);
		HWD.pfnDrawScreenFinalTexture=hwSym("DrawScreenFinalTexture",NULL);

		HWD.pfnCompileShaders   = hwSym("CompileShaders",NULL);
		HWD.pfnCleanShaders     = hwSym("CleanShaders",NULL);
		HWD.pfnSetShader        = hwSym("SetShader",NULL);
		HWD.pfnUnSetShader      = hwSym("UnSetShader",NULL);

		HWD.pfnSetShaderInfo    = hwSym("SetShaderInfo",NULL);
		HWD.pfnLoadCustomShader = hwSym("LoadCustomShader",NULL);

		vid.glstate = HWD.pfnInit() ? VID_GL_LIBRARY_LOADED : VID_GL_LIBRARY_ERROR; // let load the OpenGL library

		if (vid.glstate == VID_GL_LIBRARY_ERROR)
		{
			rendermode = render_soft;
			setrenderneeded = 0;
		}
		glstartup = true;
	}
#endif
}

//
// Splash screen
//

#ifdef SPLASH_SCREEN_SUPPORTED
static UINT32 *SplashScreen_LoadImage(const UINT8 *source, size_t source_size, UINT32 *dest_w, UINT32 *dest_h)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	png_uint_32 x, y;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	UINT32 *dest_img, *dest_img_p;

	png_io_t png_io;
	png_bytep *row_pointers;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return NULL;

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return NULL;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		return NULL;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	// set our own read function
	png_io.buffer = source;
	png_io.size = source_size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);
	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);

	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (y = 0; y < height; y++)
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, png_info_ptr));
	png_read_image(png_ptr, row_pointers);

	dest_img = (UINT32 *)(malloc(width * height * sizeof(UINT32)));
	dest_img_p = dest_img;

	for (y = 0; y < height; y++)
	{
		png_bytep row = row_pointers[y];
		for (x = 0; x < width; x++)
		{
			png_bytep px = &(row[x * 4]);
			*dest_img_p = R_PutRgbaRGBA((UINT8)px[0], (UINT8)px[1], (UINT8)px[2], (UINT8)px[3]);
			dest_img_p++;
		}
	}

	free(row_pointers);
	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	*dest_w = (UINT32)(width);
	*dest_h = (UINT32)(height);
	return dest_img;
}

static void SplashScreen_FreeImage(void)
{
	if (splashScreen.image)
	{
		free(splashScreen.image);
		splashScreen.image = NULL;
	}
}
#endif // SPLASH_SCREEN_SUPPORTED

INT32 VID_LoadSplashScreen(void)
{
#ifdef SPLASH_SCREEN_SUPPORTED
	struct SDL_RWops *file;
	Sint64 filesize;
	void *filedata;
	UINT32 swidth, sheight;
	UINT32 delay;
#endif

	Impl_InitVideoSubSystem();

#ifdef SPLASH_SCREEN_SUPPORTED
	// load splash.png
	file = SDL_RWFromFile("splash.png", "rb");
	if (!file) // not found?
	{
		CONS_Alert(CONS_ERROR, "splash screen image not found\n");
		return 0;
	}

	filesize = SDL_RWsize(file);
	if (filesize < 0) // wut?
	{
		CONS_Alert(CONS_ERROR, "error getting the file size of the splash screen image\n");
		return 0;
	}
	else if (filesize == 0)
	{
		CONS_Alert(CONS_ERROR, "the splash screen image is empty\n");
		return 0;
	}

	filedata = malloc((size_t)filesize);
	if (!filedata) // somehow couldn't malloc
	{
		CONS_Alert(CONS_ERROR, "could not find free memory for the splash screen image\n");
		return 0;
	}

	SDL_RWread(file, filedata, 1, filesize);
	SDL_RWclose(file);

	splashScreen.image = SplashScreen_LoadImage((UINT8 *)filedata, (size_t)filesize, &swidth, &sheight);
	free(filedata); // free the file data because it is not needed anymore

	if (splashScreen.image == NULL)
	{
		CONS_Alert(CONS_ERROR, "failed to read the splash screen image");
		return 0;
	}

	// create the window
	vid.width = swidth;
	vid.height = sheight;
	rendermode = render_soft;

	SDLSetMode(swidth, sheight, USE_FULLSCREEN, SDL_TRUE);

	// create a surface from the image
	bufSurface = SDL_CreateRGBSurfaceFrom(splashScreen.image, swidth, sheight, 32, (swidth * 4), 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!bufSurface)
	{
		CONS_Alert(CONS_ERROR, "could not create a surface for the splash screen image\n");
		SplashScreen_FreeImage();
		return 0;
	}

	splashScreen.displaying = SDL_TRUE;

	return 1;
#else // SPLASH_SCREEN_SUPPORTED
	return 0;
#endif
}

void VID_BlitSplashScreen(void)
{
#ifdef SPLASH_SCREEN_SUPPORTED
	if (splashScreen.displaying == SDL_TRUE)
		VID_BlitSurfaceRegion(0, 0, vid.width, vid.height);
#endif
}

void VID_PresentSplashScreen(void)
{
#ifdef SPLASH_SCREEN_SUPPORTED
	if (splashScreen.displaying == SDL_TRUE)
	{
		VID_ClearTexture();
		VID_PresentTexture();
	}
#endif
}

void I_ReportProgress(int progress)
{
	SDL_Rect base, back, front;
	const int progress_height = (vid.height / 10);
	float fprogress;

	// Offset the progress bar with the aspect ratio
	int scrw, scrh;
	float aspect[2];
	float x = 0.0f;

	SDL_GetWindowSize(window, &scrw, &scrh);

	aspect[0] = ((float)scrw) / scrh;
	aspect[1] = ((float)vid.width) / vid.height;

	if (aspect[0] < aspect[1])
		x = (aspect[0] / aspect[1]);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderClear(renderer);

	base.x = 0;
	base.y = 0;
	base.w = vid.width;
	base.h = vid.height;

	if (rendermode == render_soft || splashScreen.displaying == SDL_TRUE)
	{
		VID_BlitSurfaceRegion(0, 0, vid.width, vid.height);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
	}

	// dim screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	SDL_RenderFillRect(renderer, &base);

	// render back of progress bar
	back.x = (int)x;
	back.y = vid.height - progress_height;
	back.w = vid.width;
	back.h = progress_height;

	SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
	SDL_RenderFillRect(renderer, &back);

	// render front of progress bar
	front.x = back.x;
	front.y = back.y;
	front.h = back.h;

	fprogress = ((float)progress / 100.0f);
	front.w = (int)((float)vid.width * fprogress);

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	SDL_RenderFillRect(renderer, &front);

	// reset color
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

	// display
	SDL_RenderPresent(renderer);

	// makes the border black
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

void I_ShutdownGraphics(void)
{
	const rendermode_t oldrendermode = rendermode;

	rendermode = render_none;
	if (icoSurface) SDL_FreeSurface(icoSurface);
	icoSurface = NULL;
	if (oldrendermode == render_soft)
	{
		if (vidSurface) SDL_FreeSurface(vidSurface);
		vidSurface = NULL;
		if (vid.buffer) free(vid.buffer);
		vid.buffer = NULL;
		if (bufSurface) SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	I_OutputMsg("I_ShutdownGraphics(): ");

	// was graphics initialized anyway?
	if (!graphics_started)
	{
		I_OutputMsg("graphics never started\n");
		return;
	}
	graphics_started = false;
	I_OutputMsg("shut down\n");

#ifdef HWRENDER
#ifndef HAVE_GLES
	if (GLUhandle)
		hwClose(GLUhandle);
#endif
	if (sdlglcontext)
	{
		SDL_GL_DeleteContext(sdlglcontext);
	}
#endif
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	framebuffer = SDL_FALSE;
}
#endif
