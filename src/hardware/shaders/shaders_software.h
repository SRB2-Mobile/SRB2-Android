// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file shaders_software.h
/// \brief Software fragment shader

#define GLSL_DOOM_UNIFORMS \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n"

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
	"if (tint_color.a > 0.0) {\n" \
		"float bright = sqrt((BaseColor.r * BaseColor.r) + (BaseColor.g * BaseColor.g) + (BaseColor.b * BaseColor.b));\n" \
		"float strength = sqrt(9.0 * tint_color.a);\n" \
		"FinalColor.r = clamp((bright * (tint_color.r * strength)) + (BaseColor.r * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.g = clamp((bright * (tint_color.g * strength)) + (BaseColor.g * (1.0 - strength)), 0.0, 1.0);\n" \
		"FinalColor.b = clamp((bright * (tint_color.b * strength)) + (BaseColor.b * (1.0 - strength)), 0.0, 1.0);\n" \
	"}\n"

#define GLSL_SOFTWARE_FADE_EQUATION \
	"float darkness = R_DoomLightingEquation(lighting);\n" \
	"if (fade_start != 0.0 || fade_end != 31.0) {\n" \
		"float fs = fade_start / 31.0;\n" \
		"float fe = fade_end / 31.0;\n" \
		"float fd = fe - fs;\n" \
		"darkness = clamp((darkness - fs) * (1.0 / fd), 0.0, 1.0);\n" \
	"}\n" \
	"FinalColor = mix(FinalColor, fade_color, darkness);\n"

//
// Water surface shader
//
// Mostly guesstimated, rather than the rest being built off Software science.
// Still needs to distort things underneath/around the water...
//

#define GLSL_WATER_VARIABLES \
	"uniform float leveltime;\n" \
	"const float freq = 0.025;\n" \
	"const float amp = 0.025;\n" \
	"const float speed = 2.0;\n" \
	"const float pi = 3.14159;\n"

#define GLSL_WATER_DISTORT \
	"float z = (gl_FragCoord.z / gl_FragCoord.w) / 2.0;\n" \
	"float a = -pi * (z * freq) + (leveltime * speed);\n" \
	"float sdistort = sin(a) * amp;\n" \
	"float cdistort = cos(a) * amp;\n"

#define GLSL_WATER_MIX \
	"vec4 BaseColor = WaterTexel * poly_color;\n" \
	"vec4 FinalColor = BaseColor;\n" \
	GLSL_SOFTWARE_TINT_EQUATION \
	GLSL_SOFTWARE_FADE_EQUATION \
	"FinalColor.a = WaterTexel.a * poly_color.a;\n"
