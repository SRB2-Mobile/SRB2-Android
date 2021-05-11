// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
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

extern boolean gl_shadersenabled;
extern hwdshaderoption_t gl_allowshaders;

#ifdef GL_SHADERS

void Shader_LoadFunctions(void);

enum
{
	LOC_POSITION  = 0,
	LOC_TEXCOORD  = 1,
	LOC_NORMAL    = 2,
	LOC_COLORS    = 3,

	LOC_TEXCOORD0 = LOC_TEXCOORD,
	LOC_TEXCOORD1 = 4
};

#define MAXSHADERS 16
#define MAXSHADERPROGRAMS 16

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
	gluniform_alphatest,
	gluniform_alphathreshold,
	gluniform_isfadingin,
	gluniform_istowhite,
#endif
	gluniform_leveltime,

	gluniform_max,
} gluniform_t;

// 27072020
#ifdef HAVE_GLES2
typedef enum
{
	glattribute_position,     // LOC_POSITION
	glattribute_texcoord,     // LOC_TEXCOORD + LOC_TEXCOORD0
	glattribute_normal,       // LOC_NORMAL
	glattribute_colors,       // LOC_COLORS
	glattribute_fadetexcoord, // LOC_TEXCOORD1

	glattribute_max,
} glattribute_t;
#endif

typedef struct gl_shader_s
{
	GLuint program;
	boolean custom;

	GLint uniforms[gluniform_max+1];
#ifdef HAVE_GLES2
	GLint attributes[glattribute_max+1];
#endif

#ifdef HAVE_GLES2
	fmatrix4_t projMatrix;
	fmatrix4_t viewMatrix;
	fmatrix4_t modelMatrix;
#endif
} gl_shader_t;

extern gl_shader_t gl_shaders[HWR_MAXSHADERS];
extern gl_shader_t gl_usershaders[HWR_MAXSHADERS];
extern shadersource_t gl_customshaders[HWR_MAXSHADERS];

// 09102020
typedef struct gl_shaderstate_s
{
	gl_shader_t *current;
	GLuint type;
	GLuint program;
	boolean changed;
} gl_shaderstate_t;
extern gl_shaderstate_t gl_shaderstate;

void Shader_Set(int type);
void Shader_SetIfChanged(gl_shader_t *shader);
void Shader_UnSet(void);

#ifdef HAVE_GLES2
void Shader_SetTransform(void);
#endif

void Shader_LoadCustom(int number, char *code, size_t size, boolean isfragment);

boolean Shader_Compile(void);
void Shader_Clean(void);
void Shader_CleanPrograms(void);

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_SetSampler(gluniform_t uniform, GLint value);
#define Shader_SetIntegerUniform Shader_SetSampler
void Shader_SetInfo(hwdshaderinfo_t info, INT32 value);

#ifdef HAVE_GLES2
int Shader_AttribLoc(int loc);
const char *Shader_AttribLocName(int loc);
boolean Shader_EnableVertexAttribArray(int attrib);
boolean Shader_DisableVertexAttribArray(int attrib);
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

#endif // GL_SHADERS

#endif // _GL_SHADERS_H_
