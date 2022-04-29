// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_opengl.h
/// \brief OpenGL API for Sonic Robo Blast 2

#ifndef _R_OPENGL_H_
#define _R_OPENGL_H_

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifndef HAVE_GLES2
#include "SDL_opengl.h" //Alam_GBC: Simple, yes?
#endif

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#else
#include <GL/gl.h>
#include <GL/glu.h>

#ifdef STATIC_OPENGL // Because of the 1.3 functions, you'll need GLext to compile it if static
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#endif
#endif

#include "../../doomdef.h"
#include "../hw_drv.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#include "../r_glcommon/r_glcommon.h"

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

#ifdef USE_WGL_SWAP
typedef BOOL (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (APIENTRY *PFNWGLEXTGETSWAPINTERVALPROC) (void);
extern PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT;
extern PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT;
#endif

#endif
