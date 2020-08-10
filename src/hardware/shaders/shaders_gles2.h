// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file shaders_gl2.h
/// \brief OpenGL ES shader definitions

#include "gl_shaders.h"

// ================
//  Vertex shaders
// ================

//
// GLSL generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_ATTRIBSTATEMENT_POSITION \
	GLSL_ATTRIBSTATEMENT_TEXCOORD \
	GLSL_ATTRIBSTATEMENT_NORMAL \
	GLSL_ATTRIBSTATEMENT_COLORS \
	GLSL_LINKAGE_OUTPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_LINKAGE_OUTPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_NORMAL ";\n" \
	GLSL_LINKAGE_OUTPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_COLORS ";\n" \
	"uniform mat4 " GLSL_UNIFORM_MODEL ";\n" \
	"uniform mat4 " GLSL_UNIFORM_VIEW ";\n" \
	"uniform mat4 " GLSL_UNIFORM_PROJECTION ";\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = " GLSL_UNIFORM_PROJECTION " * " GLSL_UNIFORM_VIEW " * " GLSL_UNIFORM_MODEL " * vec4(" GLSL_ATTRIBUTE_POSITION", 1.0f);\n" \
		GLSL_LINKAGE_TEXCOORD " = vec2(" GLSL_ATTRIBUTE_TEXCOORD ".x, " GLSL_ATTRIBUTE_TEXCOORD ".y);\n" \
		GLSL_LINKAGE_NORMAL " = " GLSL_ATTRIBUTE_NORMAL ";\n" \
		GLSL_LINKAGE_COLORS " = " GLSL_ATTRIBUTE_COLORS ";\n" \
	"}\0"

//
// Fade mask vertex shader
//

#define GLSL_FADEMASK_VERTEX_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_ATTRIBSTATEMENT_POSITION \
	GLSL_ATTRIBSTATEMENT_TEXCOORD \
	GLSL_ATTRIBSTATEMENT_FADETEX \
	GLSL_LINKAGE_OUTPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_LINKAGE_OUTPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_FADEMASKTEXCOORD" ;\n" \
	"uniform mat4 " GLSL_UNIFORM_PROJECTION ";\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = " GLSL_UNIFORM_PROJECTION " * vec4(" GLSL_ATTRIBUTE_POSITION ", 1.0f);\n" \
		GLSL_LINKAGE_TEXCOORD " = vec2(" GLSL_ATTRIBUTE_TEXCOORD ".x, " GLSL_ATTRIBUTE_TEXCOORD ".y);\n" \
		GLSL_LINKAGE_FADEMASKTEXCOORD " = vec2(" GLSL_ATTRIBUTE_FADETEX ".x, " GLSL_ATTRIBUTE_FADETEX ".y);\n" \
	"}\0"

// replicates the way fixed function lighting is used by the model lighting option,
// stores the lighting result to gl_Color
// (ambient lighting of 0.75 and diffuse lighting from above)
#define GLSL_MODEL_LIGHTING_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"float nDotVP = dot(gl_Normal, vec3(0, 1, 0));\n" \
		"float light = 0.75 + max(nDotVP, 0.0);\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = vec4(light, light, light, 1.0);\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

// ==================
//  Fragment shaders
// ==================

#define GLSL_BASE_IN \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_NORMAL ";\n" \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_COLORS ";\n" \

#define GLSL_BASE_OUT \
	GLSL_COLOR_OUTPUT_STATEMENT \

#define GLSL_BASE_UNIFORMS \
	"uniform sampler2D " GLSL_UNIFORM_TEXSAMPLER ";\n" \
	"uniform vec4 " GLSL_UNIFORM_POLYCOLOR ";\n" \

//
// Generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_BASE_UNIFORMS \
	"void main(void) {\n" \
		GLSL_FRAGMENT_OUTPUT " = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_TEXSAMPLER ", " GLSL_LINKAGE_TEXCOORD ") * " GLSL_UNIFORM_POLYCOLOR ";\n" \
	"}\0"

//
// Sky fragment shader
//

#define GLSL_SKY_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_BASE_UNIFORMS \
	"void main(void) {\n" \
		GLSL_FRAGMENT_OUTPUT " = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_TEXSAMPLER ", " GLSL_LINKAGE_TEXCOORD ") * " GLSL_LINKAGE_COLORS ";\n" \
	"}\0"

//
// Fade mask fragment shader
//

#define GLSL_FADEMASK_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_COLOR_OUTPUT_STATEMENT \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_FADEMASKTEXCOORD ";\n" \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_COLORS ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_STARTSCREEN ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_ENDSCREEN ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_FADEMASK ";\n" \
	"void main(void) {\n" \
		"vec4 StartTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_STARTSCREEN ", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"vec4 EndTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_ENDSCREEN ", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"vec4 MaskTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_FADEMASK ", " GLSL_LINKAGE_FADEMASKTEXCOORD ");\n" \
		GLSL_FRAGMENT_OUTPUT " = mix(StartTexel, EndTexel, MaskTexel.r);\n" \
	"}\0"

// Lactozilla: Very simple shader that uses either additive
// or subtractive blending depending on the wipe style.
#define GLSL_FADEMASK_ADDITIVEANDSUBTRACTIVE_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_COLOR_OUTPUT_STATEMENT \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_FADEMASKTEXCOORD ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_STARTSCREEN ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_ENDSCREEN ";\n" \
	"uniform sampler2D " GLSL_UNIFORM_FADEMASK ";\n" \
	"uniform bool " GLSL_UNIFORM_ISFADINGIN ";\n" \
	"uniform bool " GLSL_UNIFORM_ISTOWHITE ";\n" \
	"void main(void) {\n" \
		"vec4 MaskTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_FADEMASK ", " GLSL_LINKAGE_FADEMASKTEXCOORD ");\n" \
		"vec4 MixTexel;\n" \
		"vec4 FinalColor;\n" \
		"float FadeAlpha = MaskTexel.r;\n" \
		"if (" GLSL_UNIFORM_ISFADINGIN " == true)\n" \
		"{\n" \
			"FadeAlpha = (1.0f - FadeAlpha);\n" \
			"MixTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_ENDSCREEN", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"}\n" \
		"else\n" \
			"MixTexel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_STARTSCREEN", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"float FadeRed = clamp((FadeAlpha * 3.0f), 0.0f, 1.0f);\n" \
		"float FadeGreen = clamp((FadeAlpha * 2.0f), 0.0f, 1.0f);\n" \
		"if (" GLSL_UNIFORM_ISTOWHITE " == true)\n" \
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
		"FinalColor.a = 1.0f;\n" \
		GLSL_FRAGMENT_OUTPUT " = FinalColor;\n" \
	"}\0"

//
// Software fragment shader
//

#define GLSL_DOOM_UNIFORMS \
	GLSL_BASE_UNIFORMS \
	"uniform vec4 " GLSL_UNIFORM_TINTCOLOR ";\n" \
	"uniform vec4 " GLSL_UNIFORM_FADECOLOR ";\n" \
	"uniform float " GLSL_UNIFORM_LIGHTING ";\n" \
	"uniform float " GLSL_UNIFORM_FADESTART ";\n" \
	"uniform float " GLSL_UNIFORM_FADEEND ";\n" \

#define GLSL_DOOM_COLORMAP \
	"float R_DoomColormap(float light, float z)\n" \
	"{\n" \
		"float lightnum = clamp(light / 17.0, 0.0, 15.0);\n" \
		"float lightz = clamp(z / 16.0, 0.0, 127.0);\n" \
		"float startmap = (15.0 - lightnum) * 4.0;\n" \
		"float scale = 160.0 / (lightz + 1.0);\n" \
		"return startmap - scale * 0.5;\n" \
	"}\n"

#define GLSL_DOOM_LIGHT_EQUATION \
	"float R_DoomLightingEquation(float light)\n" \
	"{\n" \
		"float z = gl_FragCoord.z / gl_FragCoord.w;\n" \
		"float colormap = floor(R_DoomColormap(light, z)) + 0.5;\n" \
		"return clamp(colormap, 0.0, 31.0) / 32.0;\n" \
	"}\n"

#define GLSL_SOFTWARE_TINT_EQUATION \
	"if (" GLSL_UNIFORM_TINTCOLOR ".a > 0.0) {\n" \
		"float color_bright = sqrt((BaseColor.r * BaseColor.r) + (BaseColor.g * BaseColor.g) + (BaseColor.b * BaseColor.b));\n" \
		"float strength = sqrt(9.0 * " GLSL_UNIFORM_TINTCOLOR ".a);\n" \
		"FinalColor.r = clamp((color_bright * (" GLSL_UNIFORM_TINTCOLOR ".r * strength)) + (BaseColor.r * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.g = clamp((color_bright * (" GLSL_UNIFORM_TINTCOLOR ".g * strength)) + (BaseColor.g * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.b = clamp((color_bright * (" GLSL_UNIFORM_TINTCOLOR ".b * strength)) + (BaseColor.b * (1.0 - strength)), 0.0, 1.0);\n" \
	"}\n"

#define GLSL_SOFTWARE_FADE_EQUATION \
	"float darkness = R_DoomLightingEquation(" GLSL_UNIFORM_LIGHTING ");\n" \
	"if (" GLSL_UNIFORM_FADESTART " != 0.0 || " GLSL_UNIFORM_FADEEND " != 31.0) {\n" \
		"float fs = " GLSL_UNIFORM_FADESTART " / 31.0;\n" \
		"float fe = " GLSL_UNIFORM_FADEEND " / 31.0;\n" \
		"float fd = fe - fs;\n" \
		"darkness = clamp((darkness - fs) * (1.0 / fd), 0.0, 1.0);\n" \
	"}\n" \
	"FinalColor = mix(FinalColor, " GLSL_UNIFORM_FADECOLOR ", darkness);\n"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_TEXSAMPLER ", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"vec4 BaseColor = Texel * " GLSL_UNIFORM_POLYCOLOR ";\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = Texel.a * " GLSL_UNIFORM_POLYCOLOR ".a;\n" \
		GLSL_FRAGMENT_OUTPUT " = FinalColor;\n" \
	"}\0"

// same as above but multiplies results with the lighting value from the
// accompanying vertex shader (stored in gl_Color)
#define GLSL_SOFTWARE_MODEL_LIGHTING_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 Texel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_TEXSAMPLER ", " GLSL_LINKAGE_TEXCOORD ");\n" \
		"vec4 BaseColor = Texel * " GLSL_UNIFORM_POLYCOLOR ";\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor *= " GLSL_LINKAGE_COLORS ";\n" \
		"FinalColor.a = Texel.a * PolyColor.a;\n" \
		GLSL_FRAGMENT_OUTPUT " = FinalColor;\n" \
	"}\0"

//
// Water surface shader
//
// Mostly guesstimated, rather than the rest being built off Software science.
// Still needs to distort things underneath/around the water...
//

#define GLSL_WATER_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_DOOM_UNIFORMS \
	"uniform float " GLSL_UNIFORM_LEVELTIME ";\n" \
	"const float freq = 0.025;\n" \
	"const float amp = 0.025;\n" \
	"const float speed = 2.0;\n" \
	"const float pi = 3.14159;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"float z = (gl_FragCoord.z / gl_FragCoord.w) / 2.0;\n" \
		"float a = -pi * (z * freq) + (" GLSL_UNIFORM_LEVELTIME " * speed);\n" \
		"float sdistort = sin(a) * amp;\n" \
		"float cdistort = cos(a) * amp;\n" \
		"vec4 Texel = " GLSL_TEXTURE_FUNCTION "(" GLSL_UNIFORM_TEXSAMPLER ", vec2(" GLSL_LINKAGE_TEXCOORD ".s - sdistort, " GLSL_LINKAGE_TEXCOORD ".t - cdistort));\n" \
		"vec4 BaseColor = Texel * " GLSL_UNIFORM_POLYCOLOR ";\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = Texel.a * " GLSL_UNIFORM_POLYCOLOR ".a;\n" \
		GLSL_FRAGMENT_OUTPUT " = FinalColor;\n" \
	"}\0"

//
// Fog block shader
//
// Alpha of the planes themselves are still slightly off -- see HWR_FogBlockAlpha
//

#define GLSL_FOG_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_LINKAGE_INPUT_KEYWORD " " GLSL_LINKAGEVARIABLE_TEXCOORD ";\n" \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 BaseColor = " GLSL_UNIFORM_POLYCOLOR ";\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		GLSL_FRAGMENT_OUTPUT " = FinalColor;\n" \
	"}\0"
