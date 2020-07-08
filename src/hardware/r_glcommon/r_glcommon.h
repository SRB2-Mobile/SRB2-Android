// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_glcommon.h
/// \brief Common OpenGL functions and structs shared by OpenGL backends

#ifndef _R_GLCOMMON_H_
#define _R_GLCOMMON_H_

#define GL_GLEXT_PROTOTYPES

#if defined(HAVE_GLES2)
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#elif defined(HAVE_GLES)
	#include <GLES/gl.h>
	#include <GLES/glext.h>
#else
	#ifdef HAVE_SDL
		#define _MATH_DEFINES_DEFINED

		#ifdef _MSC_VER
		#pragma warning(disable : 4214 4244)
		#endif

		#include "SDL_opengl.h" //Alam_GBC: Simple, yes?

		#ifdef _MSC_VER
		#pragma warning(default : 4214 4244)
		#endif
	#else
		#include <GL/gl.h>
		#include <GL/glu.h>
		#if defined(STATIC_OPENGL)
			#include <GL/glext.h>
		#endif
	#endif
#endif

boolean LoadGL(void);
void *GetGLFunc(const char *proc);
boolean SetupGLfunc(void);
void SetupGLFunc4(void);
void Flush(void);
INT32 isExtAvailable(const char *extension, const GLubyte *start);
void SetModelView(GLint w, GLint h);
void SetStates(void);

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

// ==========================================================================
//                                                                     GLOBAL
// ==========================================================================

extern const GLubyte	*gl_version;
extern const GLubyte	*gl_renderer;
extern const GLubyte	*gl_extensions;

extern RGBA_t           myPaletteData[];
extern GLint            screen_width;
extern GLint            screen_height;
extern GLbyte           screen_depth;
extern GLint            maximumAnisotropy;

extern GLint            textureformatGL;

struct GLRGBAFloat
{
	GLfloat red;
	GLfloat green;
	GLfloat blue;
	GLfloat alpha;
};
typedef struct GLRGBAFloat GLRGBAFloat;

#endif // _R_GLCOMMON_H_
