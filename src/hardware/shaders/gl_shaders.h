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

// Lactozilla: Set shader programs and uniforms
void *Shader_Load(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);

void Shader_Set(int shader);
void Shader_UnSet(void);
void Shader_Kill(void);

void Shader_SetInfo(hwdshaderinfo_t info, INT32 value);

boolean Shader_Compile(void);
boolean Shader_InitCustom(void);

void Shader_LoadCustom(int number, char *shader, size_t size, boolean fragment);

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
