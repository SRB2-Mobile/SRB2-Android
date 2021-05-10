// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file shaders_gles2.h
/// \brief OpenGL ES shader definitions

#include "gl_shaders.h"

#define GLSL_BASE_VARYING \
	"varying vec2 v_texcoord;\n" \
	"varying vec3 v_normal;\n" \
	"varying vec4 v_colors;\n"

// ================
//  Vertex shaders
// ================

#define GLSL_DEFAULT_VERTEX_SHADER \
	"attribute vec3 a_position;\n" \
	"attribute vec2 a_texcoord;\n" \
	"attribute vec3 a_normal;\n" \
	"attribute vec4 a_colors;\n" \
	GLSL_BASE_VARYING \
	"uniform mat4 u_model;\n" \
	"uniform mat4 u_view;\n" \
	"uniform mat4 u_projection;\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);\n" \
		"v_texcoord = vec2(a_texcoord.x, a_texcoord.y);\n" \
		"v_normal = a_normal;\n" \
		"v_colors = a_colors;\n" \
	"}\0"

//
// Fade mask vertex shader
//

#define GLSL_FADEMASK_VARYING \
	"varying vec2 v_texcoord;\n" \
	"varying vec2 v_fademasktexcoord;\n"

#define GLSL_FADEMASK_VERTEX_SHADER \
	"attribute vec3 a_position;\n" \
	"attribute vec2 a_texcoord;\n" \
	"attribute vec2 a_fademasktexcoord;\n" \
	GLSL_FADEMASK_VARYING \
	"uniform mat4 u_projection;\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = u_projection * vec4(a_position.x, a_position.y, 1.0, 1.0);\n" \
		"v_texcoord = vec2(a_texcoord.x, a_texcoord.y);\n" \
		"v_fademasktexcoord = vec2(a_fademasktexcoord.x, a_fademasktexcoord.y);\n" \
	"}\0"

// replicates the way fixed function lighting is used by the model lighting option,
// stores the lighting result to v_colors
// (ambient lighting of 0.75 and diffuse lighting from above)
#define GLSL_MODEL_LIGHTING_VERTEX_SHADER \
	"attribute vec3 a_position;\n" \
	"attribute vec2 a_texcoord;\n" \
	"attribute vec3 a_normal;\n" \
	GLSL_BASE_VARYING \
	"uniform mat4 u_model;\n" \
	"uniform mat4 u_view;\n" \
	"uniform mat4 u_projection;\n" \
	"void main()\n" \
	"{\n" \
		"float nDotVP = dot(a_normal, vec3(0.0, 1.0, 0.0));\n" \
		"float light = 0.75 + max(nDotVP, 0.0);\n" \
		"gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);\n" \
		"v_texcoord = vec2(a_texcoord.x, a_texcoord.y);\n" \
		"v_normal = a_normal;\n" \
		"v_colors = vec4(light, light, light, 1.0);\n" \
	"}\0"

// ==================
//  Fragment shaders
// ==================

#define GLSL_BASE_SAMPLER "uniform sampler2D t_texsampler;\n"

#define GLSL_BASE_UNIFORMS \
	GLSL_BASE_SAMPLER \
	"uniform vec4 poly_color;\n"

#define GLSL_PRECISION_QUALIFIER "precision mediump float;\n"

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_UNIFORMS \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(t_texsampler, v_texcoord) * poly_color;\n" \
	"}\0"

#define GLSL_DEFAULT_ALPHA_TEST \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_UNIFORMS \
	"uniform float alpha_threshold;\n" \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(t_texsampler, v_texcoord);\n" \
		"if (Texel.a <= alpha_threshold)\n" \
			"discard;\n" \
		"gl_FragColor = Texel * poly_color;\n" \
	"}\0"

//
// Sky fragment shader
//

#define GLSL_SKY_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_UNIFORMS \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(t_texsampler, v_texcoord) * v_colors;\n" \
	"}\0"

//
// Fade mask fragment shaders
//

#define GLSL_FADEMASK_BASE_IN \
	GLSL_PRECISION_QUALIFIER \
	GLSL_FADEMASK_VARYING \
	"uniform sampler2D t_startscreen;\n" \
	"uniform sampler2D t_endscreen;\n" \
	"uniform sampler2D t_fademask;\n"

#define GLSL_FADEMASK_FRAGMENT_SHADER \
	GLSL_FADEMASK_BASE_IN \
	"void main(void) {\n" \
		"vec4 StartTexel = texture2D(t_startscreen, v_texcoord);\n" \
		"vec4 EndTexel = texture2D(t_endscreen, v_texcoord);\n" \
		"vec4 MaskTexel = texture2D(t_fademask, v_fademasktexcoord);\n" \
		"gl_FragColor = mix(StartTexel, EndTexel, MaskTexel.r);\n" \
	"}\0"

// Lactozilla: Very simple shader that uses either additive
// or subtractive blending depending on the wipe style.
#define GLSL_FADEMASK_ADDITIVEANDSUBTRACTIVE_FRAGMENT_SHADER \
	GLSL_FADEMASK_BASE_IN \
	"uniform bool is_fading_in;\n" \
	"uniform bool is_to_white;\n" \
	"void main(void) {\n" \
		"vec4 MaskTexel = texture2D(t_fademask, v_fademasktexcoord);\n" \
		"vec4 MixTexel;\n" \
		"vec4 FinalColor;\n" \
		"float FadeAlpha = MaskTexel.r;\n" \
		"if (is_fading_in == true)\n" \
		"{\n" \
			"FadeAlpha = (1.0 - FadeAlpha);\n" \
			"MixTexel = texture2D(t_endscreen, v_texcoord);\n" \
		"}\n" \
		"else\n" \
			"MixTexel = texture2D(t_startscreen, v_texcoord);\n" \
		"float FadeRed = clamp((FadeAlpha * 3.0), 0.0, 1.0);\n" \
		"float FadeGreen = clamp((FadeAlpha * 2.0), 0.0, 1.0);\n" \
		"if (is_to_white == true)\n" \
		"{\n" \
			"FinalColor.r = MixTexel.r + FadeRed;\n" \
			"FinalColor.g = MixTexel.g + FadeGreen;\n" \
			"FinalColor.b = MixTexel.b + FadeAlpha;\n" \
		"}\n" \
		"else\n" \
		"{\n" \
			"FinalColor.r = MixTexel.r - FadeRed;\n" \
			"FinalColor.g = MixTexel.g - FadeGreen;\n" \
			"FinalColor.b = MixTexel.b - FadeAlpha;\n" \
		"}\n" \
		"FinalColor.a = 1.0;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Software fragment shaders
//

#include "shaders_software.h"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(t_texsampler, v_texcoord);\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

#define GLSL_SOFTWARE_ALPHA_TEST \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"uniform float alpha_threshold;\n" \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(t_texsampler, v_texcoord);\n" \
		"if (Texel.a <= alpha_threshold)\n" \
			"discard;\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Model lighting shaders
//

// same as GLSL_SOFTWARE_FRAGMENT_SHADER but multiplies results with the
// lighting value from the accompanying vertex shader (stored in v_colors)
#define GLSL_MODEL_LIGHTING_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(t_texsampler, v_texcoord);\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor *= v_colors;\n" \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

#define GLSL_MODEL_LIGHTING_ALPHA_TEST \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"uniform float alpha_threshold;\n" \
	"void main(void) {\n" \
		"vec4 Texel = texture2D(t_texsampler, v_texcoord);\n" \
		"if (Texel.a <= alpha_threshold)\n" \
			"discard;\n" \
		"vec4 BaseColor = Texel * poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor *= v_colors;\n" \
		"FinalColor.a = Texel.a * poly_color.a;\n" \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Water surface shaders
//

#define GLSL_WATER_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_WATER_VARIABLES \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		GLSL_WATER_DISTORT \
		"vec4 WaterTexel = texture2D(t_texsampler, vec2(v_texcoord.s - sdistort, v_texcoord.t - cdistort));\n" \
		GLSL_WATER_MIX \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

#define GLSL_WATER_ALPHA_TEST \
	GLSL_PRECISION_QUALIFIER \
	GLSL_BASE_VARYING \
	GLSL_BASE_SAMPLER \
	GLSL_DOOM_UNIFORMS \
	GLSL_WATER_VARIABLES \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"uniform float alpha_threshold;\n" \
	"void main(void) {\n" \
		GLSL_WATER_DISTORT \
		"vec4 WaterTexel = texture2D(t_texsampler, vec2(v_texcoord.s - sdistort, v_texcoord.t - cdistort));\n" \
		"if (WaterTexel.a <= alpha_threshold)\n" \
			"discard;\n" \
		GLSL_WATER_MIX \
		"gl_FragColor = FinalColor;\n" \
	"}\0"

//
// Fog block shader
//
// Alpha of the planes themselves are still slightly off -- see HWR_FogBlockAlpha
//

#define GLSL_FOG_FRAGMENT_SHADER \
	GLSL_PRECISION_QUALIFIER \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 BaseColor = poly_color;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"gl_FragColor = FinalColor;\n" \
	"}\0"
