// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file shaders_gl2.h
/// \brief OpenGL shader definitions

#include "gl_shaders.h"

// ================
//  Vertex shaders
// ================

//
// Generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = gl_Color;\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

// ==================
//  Fragment shaders
// ==================

//
// Generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * poly_color;\n" \
	"}\0"

// replicates the way fixed function lighting is used by the model lighting option,
// stores the lighting result to gl_Color
// (ambient lighting of 0.75 and diffuse lighting from above)
#define GLSL_MODEL_LIGHTING_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"float nDotVP = dot(gl_Normal, vec3(0.0, 1.0, 0.0));\n" \
		"float light = 0.75 + max(nDotVP, 0.0);\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = vec4(light, light, light, 1.0);\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

//
// Software fragment shader
//

#include "shaders_software.h"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

// same as above but multiplies results with the lighting value from the
// accompanying vertex shader (stored in gl_Color)
#define GLSL_MODEL_LIGHTING_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor *= gl_Color;\n" \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Water surface shader
//

#define GLSL_WATER_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	GLSL_DOOM_UNIFORMS \
	GLSL_WATER_VARIABLES \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		GLSL_WATER_DISTORT \
		"vec4 WaterTexel = texture2D(tex, vec2(gl_TexCoord[0].s - sdistort, gl_TexCoord[0].t - cdistort));\n" \
		GLSL_WATER_MIX \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Sky fragment shader
// Modulates poly_color with gl_Color
//

#define GLSL_SKY_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * gl_Color * poly_color;\n" \
	"}\0"

//
// Fog block shader
//
// Alpha of the planes themselves are still slightly off -- see HWR_FogBlockAlpha
//

#define GLSL_FOG_FRAGMENT_SHADER \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 BaseColor = gl_Color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"gl_FragColor = FinalColor;\n" \
	"}\0"
