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

extern boolean gl_shadersenabled;
extern hwdshaderoption_t gl_allowshaders;

void Shader_LoadFunctions(void);

#ifdef HAVE_GLES2
#define USE_GLES2_UNIFORMNAMES // :asafunny:
#endif

#ifdef HAVE_GLES3
    #define GLSL_VERSION_MACRO "#version 330 core\n"
    #define GLSL_TEXTURE_FUNCTION "texture"
    #define GLSL_USE_LAYOUT_QUALIFIER
    #define GLSL_USE_INANDOUT_QUALIFIERS
    #define GLSL_USE_FRAGCOLOR_OUTPUT
#else
    #define GLSL_VERSION_MACRO ""
    #define GLSL_TEXTURE_FUNCTION "texture2D"
    #define GLSL_USE_ATTRIBUTE_QUALIFIER
    #define GLSL_USE_VARYING_QUALIFIER
    #define GLSL_USE_GLCOLOR_OUTPUT
#endif

// Attribute names
#define GLSL_ATTRIBUTE_POSITION "AttributePosition"
#define GLSL_ATTRIBUTE_TEXCOORD "AttributeTexCoord"
#define GLSL_ATTRIBUTE_NORMAL   "AttributeNormal"
#define GLSL_ATTRIBUTE_COLORS   "AttributeColors"
#define GLSL_ATTRIBUTE_FADETEX  "AttributeFadeMaskTexCoord"

// Variables formed by attribute names and types
#define GLSL_ATTRIBVARIABLE_POSITION "vec3 " GLSL_ATTRIBUTE_POSITION
#define GLSL_ATTRIBVARIABLE_TEXCOORD "vec2 " GLSL_ATTRIBUTE_TEXCOORD
#define GLSL_ATTRIBVARIABLE_NORMAL   "vec3 " GLSL_ATTRIBUTE_NORMAL
#define GLSL_ATTRIBVARIABLE_COLORS   "vec4 " GLSL_ATTRIBUTE_COLORS
#define GLSL_ATTRIBVARIABLE_FADETEX  "vec2 " GLSL_ATTRIBUTE_FADETEX

// Uniform names
#define GLSL_UNIFORM_MODEL            "Model"
#define GLSL_UNIFORM_VIEW             "View"
#define GLSL_UNIFORM_PROJECTION       "Projection"

#define GLSL_UNIFORM_STARTSCREEN      "StartScreen"
#define GLSL_UNIFORM_ENDSCREEN        "EndScreen"
#define GLSL_UNIFORM_FADEMASK         "FadeMask"

#define GLSL_UNIFORM_TEXSAMPLER       "TexSampler"
#define GLSL_UNIFORM_STARTSCREEN      "StartScreen"
#define GLSL_UNIFORM_ENDSCREEN        "EndScreen"
#define GLSL_UNIFORM_FADEMASK         "FadeMask"

#ifdef USE_GLES2_UNIFORMNAMES
    #define GLSL_UNIFORM_POLYCOLOR    "PolyColor"
    #define GLSL_UNIFORM_TINTCOLOR    "TintColor"
    #define GLSL_UNIFORM_FADECOLOR    "FadeColor"
    #define GLSL_UNIFORM_LIGHTING     "Lighting"
    #define GLSL_UNIFORM_FADESTART    "FadeStart"
    #define GLSL_UNIFORM_FADEEND      "FadeEnd"

    #define GLSL_UNIFORM_ISFADINGIN   "IsFadingIn"
    #define GLSL_UNIFORM_ISTOWHITE    "IsToWhite"
    #define GLSL_UNIFORM_LEVELTIME    "LevelTime"
#else
    #define GLSL_UNIFORM_POLYCOLOR    "poly_color"
    #define GLSL_UNIFORM_TINTCOLOR    "tint_color"
    #define GLSL_UNIFORM_FADECOLOR    "fade_color"
    #define GLSL_UNIFORM_LIGHTING     "lighting"
    #define GLSL_UNIFORM_FADESTART    "fade_start"
    #define GLSL_UNIFORM_FADEEND      "fade_end"

    #define GLSL_UNIFORM_LEVELTIME    "leveltime"
#endif

// Linkage
#define GLSL_LINKAGE_TEXCOORD                 "TexCoord"
#define GLSL_LINKAGE_NORMAL                   "Normal"
#define GLSL_LINKAGE_COLORS                   "Colors"
#define GLSL_LINKAGE_FADEMASKTEXCOORD         "FadeMaskTexCoord"

#define GLSL_LINKAGEVARIABLE_TEXCOORD         "vec2 " GLSL_LINKAGE_TEXCOORD
#define GLSL_LINKAGEVARIABLE_NORMAL           "vec3 " GLSL_LINKAGE_NORMAL
#define GLSL_LINKAGEVARIABLE_COLORS           "vec4 " GLSL_LINKAGE_COLORS
#define GLSL_LINKAGEVARIABLE_FADEMASKTEXCOORD "vec2 " GLSL_LINKAGE_FADEMASKTEXCOORD

// ---------------------------------------

// Use the layout qualifier to specify vertex attributes.
#ifdef GLSL_USE_LAYOUT_QUALIFIER // GLSL ES 300
    #define GLSL_ATTRIBSTATEMENT_POSITION "layout (location = 0) in " GLSL_ATTRIBVARIABLE_POSITION ";\n"
    #define GLSL_ATTRIBSTATEMENT_TEXCOORD "layout (location = 1) in " GLSL_ATTRIBVARIABLE_TEXCOORD ";\n"
    #define GLSL_ATTRIBSTATEMENT_NORMAL   "layout (location = 2) in " GLSL_ATTRIBVARIABLE_NORMAL ";\n"
    #define GLSL_ATTRIBSTATEMENT_COLORS   "layout (location = 3) in " GLSL_ATTRIBVARIABLE_COLORS ";\n"
    #define GLSL_ATTRIBSTATEMENT_FADETEX  "layout (location = 4) in " GLSL_ATTRIBVARIABLE_FADETEX ";\n"
#else // GLSL ES 100 (GLSL_USE_VARYING_QUALIFIER)
    // Use the attribute qualifier to specify vertex attributes.
    #define GLSL_ATTRIBSTATEMENT_POSITION "attribute " GLSL_ATTRIBVARIABLE_POSITION ";\n"
    #define GLSL_ATTRIBSTATEMENT_TEXCOORD "attribute " GLSL_ATTRIBVARIABLE_TEXCOORD ";\n"
    #define GLSL_ATTRIBSTATEMENT_NORMAL   "attribute " GLSL_ATTRIBVARIABLE_NORMAL ";\n"
    #define GLSL_ATTRIBSTATEMENT_COLORS   "attribute " GLSL_ATTRIBVARIABLE_COLORS ";\n"
    #define GLSL_ATTRIBSTATEMENT_FADETEX  "attribute " GLSL_ATTRIBVARIABLE_FADETEX ";\n"
#endif

// ---------------------------------------

// Use the 'in' and 'out' qualifiers.
#ifdef GLSL_USE_INANDOUT_QUALIFIERS
    #define GLSL_LINKAGE_INPUT_KEYWORD  "in"
    #define GLSL_LINKAGE_OUTPUT_KEYWORD "out"
#else // GLSL_USE_VARYING_QUALIFIER
    // Use the 'varying' qualifier.
    #define GLSL_LINKAGE_INPUT_KEYWORD  "varying"
    #define GLSL_LINKAGE_OUTPUT_KEYWORD "varying"
#endif

// ---------------------------------------

#ifdef GLSL_USE_FRAGCOLOR_OUTPUT
    // Use 'FragColor' to output the fragment's color.
    #define GLSL_FRAGMENT_OUTPUT             "FragColor"
    #define GLSL_COLOR_OUTPUT_STATEMENT      GLSL_LINKAGE_OUTPUT_KEYWORD " vec4 FragColor;\n"
#else // GLSL_USE_GLCOLOR_OUTPUT
    // Use 'gl_FragColor' to output the fragment's color.
    #define GLSL_FRAGMENT_OUTPUT             "gl_FragColor"
    #define GLSL_COLOR_OUTPUT_STATEMENT      ""
#endif

// ---------------------------------------

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

// 27072020
#ifdef GLSL_USE_ATTRIBUTE_QUALIFIER
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
#ifdef GLSL_USE_ATTRIBUTE_QUALIFIER
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
void Shader_UnSet(void);

#ifdef HAVE_GLES2
void Shader_SetTransform(void);
#endif

void Shader_LoadCustom(int number, char *code, size_t size, boolean isfragment);

boolean Shader_Compile(void);
void Shader_Clean(void);

void Shader_SetColors(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_SetSampler(gl_shader_t *shader, gluniform_t uniform, GLint value);
#define Shader_SetIntegerUniform Shader_SetSampler
void Shader_SetInfo(hwdshaderinfo_t info, INT32 value);

#if defined(GLSL_USE_ATTRIBUTE_QUALIFIER)
int Shader_AttribLoc(int loc);
#elif defined(GLSL_USE_LAYOUT_QUALIFIER)
#define Shader_AttribLoc(x) (x)
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
