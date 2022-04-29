// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_gles.h
/// \brief OpenGL ES API for Sonic Robo Blast 2

#ifndef _R_GLES_H_
#define _R_GLES_H_

#define GL_GLEXT_PROTOTYPES
#undef DRIVER_STRING

#ifdef HAVE_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define DRIVER_STRING "OpenGL ES 1.1"
#else
#include <GLES/gl.h>
#include <GLES/glext.h>
#define DRIVER_STRING "OpenGL ES 2.0"
#endif

#undef DEBUG_TO_FILE

#include "../../doomdef.h"
#include "../hw_drv.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#include "../r_glcommon/r_glcommon.h"

#endif
