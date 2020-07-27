// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file shaders_gl2.h
/// \brief OpenGL ES shader definitions

#define GLSL_VERSION_MACRO "#version 330 core\n"

// ================
//  Vertex shaders
// ================

//
// GLSL generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	GLSL_VERSION_MACRO \
	"layout (location = 0) in vec3 aPos;\n" \
	"layout (location = 1) in vec2 aTexCoord;\n" \
	"layout (location = 2) in vec3 aNormal;\n" \
	"layout (location = 3) in vec4 aColors;\n" \
	"out vec2 TexCoord;\n" \
	"out vec3 Normal;\n" \
	"out vec4 Colors;\n" \
	"uniform mat4 Model;\n" \
	"uniform mat4 View;\n" \
	"uniform mat4 Projection;\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = Projection * View * Model * vec4(aPos, 1.0f);\n" \
		"TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n" \
		"Normal = aNormal;\n" \
		"Colors = aColors;\n" \
	"}\0"

//
// Fade mask vertex shader
//

#define GLSL_FADEMASK_VERTEX_SHADER \
	GLSL_VERSION_MACRO \
	"layout (location = 0) in vec3 aPos;\n" \
	"layout (location = 1) in vec2 aTexCoord;\n" \
	"layout (location = 2) in vec2 aFadeMaskTexCoord;\n" \
	"out vec2 TexCoord;\n" \
	"out vec2 FadeMaskTexCoord;\n" \
	"uniform mat4 Projection;\n" \
	"void main()\n" \
	"{\n" \
		"gl_Position = Projection * vec4(aPos, 1.0f);\n" \
		"TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n" \
		"FadeMaskTexCoord = vec2(aFadeMaskTexCoord.x, aFadeMaskTexCoord.y);\n" \
	"}\0"

// ==================
//  Fragment shaders
// ==================

#define GLSL_BASE_IN \
	"in vec2 TexCoord;\n" \
	"in vec3 Normal;\n" \
	"in vec4 Colors;\n" \

#define GLSL_BASE_OUT \
	"out vec4 FragColor;\n" \

#define GLSL_BASE_UNIFORMS \
	"uniform sampler2D TexSampler;\n" \
	"uniform vec4 PolyColor;\n" \

//
// Generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_BASE_UNIFORMS \
	"void main(void) {\n" \
		"FragColor = texture(TexSampler, TexCoord) * PolyColor;\n" \
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
		"FragColor = texture(TexSampler, TexCoord) * Colors;\n" \
	"}\0"

//
// Fade mask fragment shader
//

#define GLSL_FADEMASK_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	"out vec4 FragColor;\n" \
	"in vec2 TexCoord;\n" \
	"in vec2 FadeMaskTexCoord;\n" \
	"uniform sampler2D StartScreen;\n" \
	"uniform sampler2D EndScreen;\n" \
	"uniform sampler2D FadeMask;\n" \
	"void main(void) {\n" \
		"vec4 StartTexel = texture(StartScreen, TexCoord);\n" \
		"vec4 EndTexel = texture(EndScreen, TexCoord);\n" \
		"vec4 MaskTexel = texture(FadeMask, FadeMaskTexCoord);\n" \
		"FragColor = mix(StartTexel, EndTexel, MaskTexel.r);\n" \
	"}\0"

// Lactozilla: Very simple shader that uses either additive
// or subtractive blending depending on the wipe style.
#define GLSL_FADEMASK_ADDITIVEANDSUBTRACTIVE_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	"out vec4 FragColor;\n" \
	"in vec2 TexCoord;\n" \
	"in vec2 FadeMaskTexCoord;\n" \
	"uniform sampler2D StartScreen;\n" \
	"uniform sampler2D EndScreen;\n" \
	"uniform sampler2D FadeMask;\n" \
	"uniform bool IsFadingIn;\n" \
	"uniform bool IsToWhite;\n" \
	"void main(void) {\n" \
		"vec4 MaskTexel = texture(FadeMask, FadeMaskTexCoord);\n" \
		"vec4 MixTexel;\n" \
		"vec4 FinalColor;\n" \
		"float FadeAlpha = MaskTexel.r;\n" \
		"if (IsFadingIn == true)\n" \
		"{\n" \
			"FadeAlpha = (1.0f - FadeAlpha);\n" \
			"MixTexel = texture(EndScreen, TexCoord);\n" \
		"}\n" \
		"else\n" \
			"MixTexel = texture(StartScreen, TexCoord);\n" \
		"float FadeRed = clamp((FadeAlpha * 3.0f), 0.0f, 1.0f);\n" \
		"float FadeGreen = clamp((FadeAlpha * 2.0f), 0.0f, 1.0f);\n" \
		"if (IsToWhite == true)\n" \
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
		"FragColor = FinalColor;\n" \
	"}\0"

//
// Software fragment shader
//

#define GLSL_DOOM_UNIFORMS \
	GLSL_BASE_UNIFORMS \
	"uniform vec4 TintColor;\n" \
	"uniform vec4 FadeColor;\n" \
	"uniform float Lighting;\n" \
	"uniform float FadeStart;\n" \
	"uniform float FadeEnd;\n" \

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
	"if (TintColor.a > 0.0) {\n" \
		"float color_bright = sqrt((BaseColor.r * BaseColor.r) + (BaseColor.g * BaseColor.g) + (BaseColor.b * BaseColor.b));\n" \
		"float strength = sqrt(9.0 * TintColor.a);\n" \
		"FinalColor.r = clamp((color_bright * (TintColor.r * strength)) + (BaseColor.r * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.g = clamp((color_bright * (TintColor.g * strength)) + (BaseColor.g * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.b = clamp((color_bright * (TintColor.b * strength)) + (BaseColor.b * (1.0 - strength)), 0.0, 1.0);\n" \
	"}\n"

#define GLSL_SOFTWARE_FADE_EQUATION \
	"float darkness = R_DoomLightingEquation(Lighting);\n" \
	"if (FadeStart != 0.0 || FadeEnd != 31.0) {\n" \
		"float fs = FadeStart / 31.0;\n" \
		"float fe = FadeEnd / 31.0;\n" \
		"float fd = fe - fs;\n" \
		"darkness = clamp((darkness - fs) * (1.0 / fd), 0.0, 1.0);\n" \
	"}\n" \
	"FinalColor = mix(FinalColor, FadeColor, darkness);\n"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	GLSL_BASE_IN \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 texel = texture2D(TexSampler, TexCoord);\n" \
		"vec4 BaseColor = texel * PolyColor;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = texel.a * PolyColor.a;\n" \
		"FragColor = FinalColor;\n" \
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
	"in vec2 TexCoord;\n" \
	GLSL_DOOM_UNIFORMS \
	"uniform float LevelTime;\n" \
	"const float freq = 0.025;\n" \
	"const float amp = 0.025;\n" \
	"const float speed = 2.0;\n" \
	"const float pi = 3.14159;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"float z = (gl_FragCoord.z / gl_FragCoord.w) / 2.0;\n" \
		"float a = -pi * (z * freq) + (LevelTime * speed);\n" \
		"float sdistort = sin(a) * amp;\n" \
		"float cdistort = cos(a) * amp;\n" \
		"vec4 texel = texture(TexSampler, vec2(TexCoord.s - sdistort, TexCoord.t - cdistort));\n" \
		"vec4 BaseColor = texel * PolyColor;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FinalColor.a = texel.a * PolyColor.a;\n" \
		"FragColor = FinalColor;\n" \
	"}\0"

//
// Fog block shader
//
// Alpha of the planes themselves are still slightly off -- see HWR_FogBlockAlpha
//

#define GLSL_FOG_FRAGMENT_SHADER \
	GLSL_VERSION_MACRO \
	GLSL_BASE_OUT \
	"in vec2 TexCoord;\n" \
	GLSL_DOOM_UNIFORMS \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 BaseColor = PolyColor;\n" \
		"vec4 FinalColor = BaseColor;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"FragColor = FinalColor;\n" \
	"}\0"
