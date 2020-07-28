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

#include "../../doomdata.h"
#include "../../doomtype.h"
#include "../../doomdef.h"

boolean LoadGL(void);
void *GetGLFunc(const char *proc);
boolean SetupGLfunc(void);
void SetupGLFunc4(void);
void Flush(void);
INT32 isExtAvailable(const char *extension, const GLubyte *start);
void SetModelView(GLint w, GLint h);
void SetStates(void);

void GL_DBG_Printf(const char *format, ...);
void GL_MSG_Warning(const char *format, ...);
void GL_MSG_Error(const char *format, ...);

#ifdef DEBUG_TO_FILE
extern FILE *gllogstream;
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef R_GL_APIENTRY
	#if defined(_WIN32)
		#define R_GL_APIENTRY APIENTRY
	#else
		#define R_GL_APIENTRY
	#endif
#endif

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

#if defined(STATIC_OPENGL) && !defined(HAVE_GLES)
#define pglClear glClear
#define pglGetIntegerv glGetIntegerv
#define pglGetString glGetString
#else
/* 1.0 Miscellaneous functions */
typedef void (R_GL_APIENTRY * PFNglClear) (GLbitfield mask);
extern PFNglClear pglClear;
typedef void (R_GL_APIENTRY * PFNglGetIntegerv) (GLenum pname, GLint *params);
extern PFNglGetIntegerv pglGetIntegerv;
typedef const GLubyte * (R_GL_APIENTRY * PFNglGetString) (GLenum name);
extern PFNglGetString pglGetString;
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

#ifdef HAVE_GLES2
typedef float fvector3_t[3];
typedef float fvector4_t[4];
typedef fvector4_t fmatrix4_t[4];

extern fmatrix4_t projMatrix;
extern fmatrix4_t viewMatrix;
extern fmatrix4_t modelMatrix;
#endif

/* 1.2 Parms */
/* GL_CLAMP_TO_EDGE_EXT */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_MIN_LOD
#define GL_TEXTURE_MIN_LOD 0x813A
#endif
#ifndef GL_TEXTURE_MAX_LOD
#define GL_TEXTURE_MAX_LOD 0x813B
#endif

/* 1.3 GL_TEXTUREi */
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

/* 1.5 Parms */
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif

#endif // _R_GLCOMMON_H_
