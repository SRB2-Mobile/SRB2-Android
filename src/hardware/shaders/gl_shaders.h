// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file gl_shaders.h
/// \brief OpenGL shaders

#ifndef _GL_SHADERS_H_
#define _GL_SHADERS_H_

#include "../../doomdef.h"
#include "../hw_drv.h"

#include "../r_glcommon/r_glcommon.h"

#ifndef R_GL_APIENTRY
	#if defined(_WIN32)
		#define R_GL_APIENTRY APIENTRY
	#else
		#define R_GL_APIENTRY
	#endif
#endif

extern boolean gl_allowshaders;
extern boolean gl_shadersenabled;

void Shader_SetupGLFunc(void);

#ifdef HAVE_GLES2
static boolean Shader_SetProgram(gl_shaderprogram_t *shader);
static void Shader_SetTransform(void);
#else
void *Shader_Load(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_Set(int shader);
void Shader_UnSet(void);
#endif

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);

void Shader_SetInfo(hwdshaderinfo_t info, INT32 value);

boolean Shader_Compile(void);
boolean Shader_InitCustom(void);

void Shader_LoadCustom(int number, char *shader, size_t size, boolean fragment);

#define MAXSHADERS 16
#define MAXSHADERPROGRAMS 16

// 18032019
extern char *gl_customvertexshaders[MAXSHADERS];
extern char *gl_customfragmentshaders[MAXSHADERS];
extern GLuint gl_currentshaderprogram;
extern boolean gl_shaderprogramchanged;

// 08072020
typedef enum
{
#ifdef HAVE_GLES2
	// transform
	gluniform_model,
	gluniform_view,
	gluniform_projection,

	// samplers
	gluniform_startscreen,
	gluniform_endscreen,
	gluniform_fademask,
#endif

	// lighting
	gluniform_poly_color,
	gluniform_tint_color,
	gluniform_fade_color,
	gluniform_lighting,
	gluniform_fade_start,
	gluniform_fade_end,

	// misc.
#ifdef HAVE_GLES2
	gluniform_isfadingin,
	gluniform_istowhite,
#endif
	gluniform_leveltime,

	gluniform_max,
} gluniform_t;

typedef struct gl_shaderprogram_s
{
	GLuint program;
	boolean custom;
	GLint uniforms[gluniform_max+1];

#ifdef HAVE_GLES2
	fmatrix4_t projMatrix;
	fmatrix4_t viewMatrix;
	fmatrix4_t modelMatrix;
#endif
} gl_shaderprogram_t;
extern gl_shaderprogram_t gl_shaderprograms[MAXSHADERPROGRAMS];

#ifdef HAVE_GLES2
extern gl_shaderprogram_t *shader_base;
extern gl_shaderprogram_t *shader_current;
#endif

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

#endif // _GL_SHADERS_H_
