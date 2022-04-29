// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2021 by Sonic Team Junior.
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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SDL specific part of the OpenGL ES API for SRB2

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#include "SDL.h"

#include "sdlmain.h"

#include "../doomdef.h"

#ifdef HWRENDER
#include "../hardware/r_gles/r_gles.h"
#include "ogl_es_sdl.h"
#include "../i_system.h"
#include "hwsym_sdl.h"
#include "../m_argv.h"

PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;

/**	\brief SDL video display surface
*/
SDL_GLContext sdlglcontext = 0;

void *GLBackend_GetFunction(const char *proc)
{
	return SDL_GL_GetProcAddress(proc);
}

boolean GLBackend_Init(void)
{
	if (SDL_GL_LoadLibrary(NULL) != 0)
	{
		CONS_Alert(CONS_ERROR, "Could not load OpenGL Library: %s\nFalling back to Software mode.\n", SDL_GetError());
		return 0;
	}

	if (!GLBackend_InitContext())
		return false;

	return GLBackend_LoadFunctions();
}

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height

	\return	if true, changed video mode
*/
boolean OglSdlSurface(INT32 w, INT32 h)
{
	// Lactozilla: Does exactly just that in ES :D
	GLBackend_SetSurface(w, h);
	return true;
}

#ifdef HAVE_GL_FRAMEBUFFER
static boolean firstFramebuffer = false;
#endif

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	int sdlw, sdlh;

	static boolean oldwaitvbl = false;
	if (oldwaitvbl != waitvbl)
		SDL_GL_SetSwapInterval(waitvbl ? 1 : 0);

	oldwaitvbl = waitvbl;

	SDL_GetWindowSize(window, &sdlw, &sdlh);
	GPU->MakeFinalScreenTexture();

#ifdef HAVE_GL_FRAMEBUFFER
	GLFramebuffer_Disable();
	RenderToFramebuffer = FramebufferEnabled;
#endif

	GPU->DrawFinalScreenTexture(sdlw, sdlh);

#ifdef HAVE_GL_FRAMEBUFFER
	if (RenderToFramebuffer)
	{
		// I have no idea why I have to do this.
		if (!firstFramebuffer)
		{
			GPU->SetBlend(PF_Translucent|PF_Occlude|PF_Masked);
			firstFramebuffer = true;
		}

		GLFramebuffer_Enable();
	}
#endif

	SDL_GL_SwapWindow(window);
	GPU->GClipRect(0, 0, realwidth, realheight, NZCLIP_PLANE);

	// Sryder:	We need to draw the final screen texture again into the other buffer in the original position so that
	//			effects that want to take the old screen can do so after this
	GPU->DrawFinalScreenTexture(realwidth, realheight);
}

#endif //HWRENDER
#endif //SDL
