// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file gl_shaders.c
/// \brief OpenGL shaders

#include "gl_shaders.h"
#include "../r_glcommon/r_glcommon.h"

// NOTE: HAVE_GLES2 should imply GL_SHADERS

boolean gl_allowshaders = false;
boolean gl_shadersenabled = false;

#ifdef GL_SHADERS
typedef GLuint (R_GL_APIENTRY *PFNglCreateShader)       (GLenum);
typedef void   (R_GL_APIENTRY *PFNglShaderSource)       (GLuint, GLsizei, const GLchar**, GLint*);
typedef void   (R_GL_APIENTRY *PFNglCompileShader)      (GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetShaderiv)        (GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglGetShaderInfoLog)   (GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (R_GL_APIENTRY *PFNglDeleteShader)       (GLuint);
typedef GLuint (R_GL_APIENTRY *PFNglCreateProgram)      (void);
typedef void   (R_GL_APIENTRY *PFNglAttachShader)       (GLuint, GLuint);
typedef void   (R_GL_APIENTRY *PFNglLinkProgram)        (GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetProgramiv)       (GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglUseProgram)         (GLuint);
typedef void   (R_GL_APIENTRY *PFNglUniform1i)          (GLint, GLint);
typedef void   (R_GL_APIENTRY *PFNglUniform1f)          (GLint, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform2f)          (GLint, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform3f)          (GLint, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform4f)          (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform1fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform2fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform3fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniformMatrix4fv)   (GLint, GLsizei, GLboolean, const GLfloat *);
typedef GLint  (R_GL_APIENTRY *PFNglGetUniformLocation) (GLuint, const GLchar*);
typedef GLint  (R_GL_APIENTRY *PFNglGetAttribLocation)  (GLuint, const GLchar*);

static PFNglCreateShader pglCreateShader;
static PFNglShaderSource pglShaderSource;
static PFNglCompileShader pglCompileShader;
static PFNglGetShaderiv pglGetShaderiv;
static PFNglGetShaderInfoLog pglGetShaderInfoLog;
static PFNglDeleteShader pglDeleteShader;
static PFNglCreateProgram pglCreateProgram;
static PFNglAttachShader pglAttachShader;
static PFNglLinkProgram pglLinkProgram;
static PFNglGetProgramiv pglGetProgramiv;
static PFNglUseProgram pglUseProgram;
static PFNglUniform1i pglUniform1i;
static PFNglUniform1f pglUniform1f;
static PFNglUniform2f pglUniform2f;
static PFNglUniform3f pglUniform3f;
static PFNglUniform4f pglUniform4f;
static PFNglUniform1fv pglUniform1fv;
static PFNglUniform2fv pglUniform2fv;
static PFNglUniform3fv pglUniform3fv;
static PFNglUniformMatrix4fv pglUniformMatrix4fv;
static PFNglGetUniformLocation pglGetUniformLocation;
static PFNglGetAttribLocation pglGetAttribLocation;

char *gl_customvertexshaders[MAXSHADERS];
char *gl_customfragmentshaders[MAXSHADERS];
GLuint gl_currentshaderprogram = 0;
boolean gl_shaderprogramchanged = true;

gl_shaderprogram_t gl_shaderprograms[MAXSHADERPROGRAMS];

#ifdef HAVE_GLES2
gl_shaderprogram_t *shader_base = NULL;
gl_shaderprogram_t *shader_current = NULL;
#endif

// Shader info
static INT32 shader_leveltime = 0;

#ifdef HAVE_GLES2
#include "shaders_gles2.h"
#else
#include "shaders_gl2.h"
#endif

static const char *vertex_shaders[] = {
	// Default vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Floor vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Wall vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Sprite vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Model vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Water vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Fog vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Sky vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

#ifdef HAVE_GLES2
	// Fade mask vertex shader
	GLSL_FADEMASK_VERTEX_SHADER, GLSL_FADEMASK_VERTEX_SHADER,
#endif

	NULL,
};

static const char *fragment_shaders[] = {
	// Default fragment shader
	GLSL_DEFAULT_FRAGMENT_SHADER,

	// Floor fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Wall fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Sprite fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Model fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Water fragment shader
	GLSL_WATER_FRAGMENT_SHADER,

	// Fog fragment shader
	GLSL_FOG_FRAGMENT_SHADER,

	// Sky fragment shader
#ifdef HAVE_GLES2
	GLSL_SKY_FRAGMENT_SHADER,
#else
	"uniform sampler2D tex;\n"
	"void main(void) {\n"
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"}\0",
#endif

#ifdef HAVE_GLES2
	// Fade mask vertex shader
	GLSL_FADEMASK_FRAGMENT_SHADER, GLSL_FADEMASK_ADDITIVEANDSUBTRACTIVE_FRAGMENT_SHADER,
#endif

	NULL,
};

#endif	// GL_SHADERS

void Shader_SetupGLFunc(void)
{
#ifdef GL_SHADERS
	pglCreateShader = GetGLFunc("glCreateShader");
	pglShaderSource = GetGLFunc("glShaderSource");
	pglCompileShader = GetGLFunc("glCompileShader");
	pglGetShaderiv = GetGLFunc("glGetShaderiv");
	pglGetShaderInfoLog = GetGLFunc("glGetShaderInfoLog");
	pglDeleteShader = GetGLFunc("glDeleteShader");
	pglCreateProgram = GetGLFunc("glCreateProgram");
	pglAttachShader = GetGLFunc("glAttachShader");
	pglLinkProgram = GetGLFunc("glLinkProgram");
	pglGetProgramiv = GetGLFunc("glGetProgramiv");
	pglUseProgram = GetGLFunc("glUseProgram");
	pglUniform1i = GetGLFunc("glUniform1i");
	pglUniform1f = GetGLFunc("glUniform1f");
	pglUniform2f = GetGLFunc("glUniform2f");
	pglUniform3f = GetGLFunc("glUniform3f");
	pglUniform4f = GetGLFunc("glUniform4f");
	pglUniform1fv = GetGLFunc("glUniform1fv");
	pglUniform2fv = GetGLFunc("glUniform2fv");
	pglUniform3fv = GetGLFunc("glUniform3fv");
	pglUniformMatrix4fv = GetGLFunc("glUniformMatrix4fv");
	pglGetUniformLocation = GetGLFunc("glGetUniformLocation");
	pglGetAttribLocation = GetGLFunc("glGetAttribLocation");
#endif
}

//
// Shader info
// Those are given to the uniforms.
//

void Shader_SetInfo(hwdshaderinfo_t info, INT32 value)
{
#ifdef GL_SHADERS
	switch (info)
	{
		case HWD_SHADERINFO_LEVELTIME:
			shader_leveltime = value;
			break;
		default:
			break;
	}
#else
	(void)info;
	(void)value;
#endif
}

//
// Custom shader loading
//
void Shader_LoadCustom(int number, char *shader, size_t size, boolean fragment)
{
#ifdef GL_SHADERS
	if (!pglUseProgram) return;
	if (number < 1 || number > MAXSHADERS)
		I_Error("LoadCustomShader(): cannot load shader %d (max %d)", number, MAXSHADERS);

	if (fragment)
	{
		gl_customfragmentshaders[number] = malloc(size+1);
		strncpy(gl_customfragmentshaders[number], shader, size);
		gl_customfragmentshaders[number][size] = 0;
	}
	else
	{
		gl_customvertexshaders[number] = malloc(size+1);
		strncpy(gl_customvertexshaders[number], shader, size);
		gl_customvertexshaders[number][size] = 0;
	}
#else
	(void)number;
	(void)shader;
	(void)size;
	(void)fragment;
#endif
}

boolean Shader_InitCustom(void)
{
#ifdef GL_SHADERS
	return Shader_Compile();
#else
	return false;
#endif
}

void Shader_Set(int shader)
{
#ifdef HAVE_GLES2
	if (Shader_SetProgram((gl_shadersenabled) ? (&gl_shaderprograms[shader]) : shader_base))
		Shader_SetTransform();
	return;
#elif defined(GL_SHADERS)
	if (gl_allowshaders)
	{
		if ((GLuint)shader != gl_currentshaderprogram)
		{
			gl_currentshaderprogram = shader;
			gl_shaderprogramchanged = true;
		}
		gl_shadersenabled = true;
		return;
	}
#else
	(void)shader;
#endif
	gl_shadersenabled = false;
}

void Shader_UnSet(void)
{
#ifdef HAVE_GLES2
	GLRGBAFloat white = {1.0f, 1.0f, 1.0f, 1.0f};

	shader_current = shader_base;
	pglUseProgram(shader_base->program);

	Shader_SetUniforms(NULL, &white, NULL, NULL);
#elif defined(GL_SHADERS)
	gl_shadersenabled = false;
	gl_currentshaderprogram = 0;

	if (pglUseProgram)
		pglUseProgram(0);
#endif
}

#ifdef HAVE_GLES2
boolean Shader_SetProgram(gl_shaderprogram_t *shader)
{
	if (shader != shader_current)
	{
		shader_current = shader;
		pglUseProgram(shader->program);
		return true;
	}
	return false;
}
#endif

#ifdef HAVE_GLES2
#define Shader_CompileError I_Error
#else
#define Shader_CompileError GL_MSG_Error
#endif

boolean Shader_Compile(void)
{
#ifdef GL_SHADERS
	GLuint gl_vertShader, gl_fragShader;
	GLint i, result;

	if (!pglUseProgram) return false;

	gl_customvertexshaders[0] = NULL;
	gl_customfragmentshaders[0] = NULL;

#ifdef HAVE_GLES2
	shader_base = shader_current = &gl_shaderprograms[0];
#endif

	for (i = 0; vertex_shaders[i] && fragment_shaders[i]; i++)
	{
		gl_shaderprogram_t *shader;
		const GLchar* vert_shader = vertex_shaders[i];
		const GLchar* frag_shader = fragment_shaders[i];
		boolean custom = ((gl_customvertexshaders[i] || gl_customfragmentshaders[i]) && (i > 0));

		// 18032019
		if (gl_customvertexshaders[i])
			vert_shader = gl_customvertexshaders[i];
		if (gl_customfragmentshaders[i])
			frag_shader = gl_customfragmentshaders[i];

		if (i >= MAXSHADERS)
			break;
		if (i >= MAXSHADERPROGRAMS)
			break;

		shader = &gl_shaderprograms[i];
		shader->program = 0;
		shader->custom = custom;

		//
		// Load and compile vertex shader
		//
		gl_vertShader = pglCreateShader(GL_VERTEX_SHADER);
		if (!gl_vertShader)
		{
			Shader_CompileError("Shader_Compile: Error creating vertex shader %d\n", i);
			continue;
		}

		pglShaderSource(gl_vertShader, 1, &vert_shader, NULL);
		pglCompileShader(gl_vertShader);

		// check for compile errors
		pglGetShaderiv(gl_vertShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			GLchar* infoLog;
			GLint logLength;

			pglGetShaderiv(gl_vertShader, GL_INFO_LOG_LENGTH, &logLength);

			infoLog = malloc(logLength);
			pglGetShaderInfoLog(gl_vertShader, logLength, NULL, infoLog);

			Shader_CompileError("Shader_Compile: Error compiling vertex shader %d\n%s", i, infoLog);
			continue;
		}

		//
		// Load and compile fragment shader
		//
		gl_fragShader = pglCreateShader(GL_FRAGMENT_SHADER);
		if (!gl_fragShader)
		{
			Shader_CompileError("Shader_Compile: Error creating fragment shader %d\n", i);
			continue;
		}

		pglShaderSource(gl_fragShader, 1, &frag_shader, NULL);
		pglCompileShader(gl_fragShader);

		// check for compile errors
		pglGetShaderiv(gl_fragShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			GLchar* infoLog;
			GLint logLength;

			pglGetShaderiv(gl_fragShader, GL_INFO_LOG_LENGTH, &logLength);

			infoLog = malloc(logLength);
			pglGetShaderInfoLog(gl_fragShader, logLength, NULL, infoLog);

			Shader_CompileError("Shader_Compile: Error compiling fragment shader %d\n%s", i, infoLog);
			continue;
		}

		shader->program = pglCreateProgram();
		pglAttachShader(shader->program, gl_vertShader);
		pglAttachShader(shader->program, gl_fragShader);
		pglLinkProgram(shader->program);

		// check link status
		pglGetProgramiv(shader->program, GL_LINK_STATUS, &result);

		// delete the shader objects
		pglDeleteShader(gl_vertShader);
		pglDeleteShader(gl_fragShader);

		// couldn't link?
		if (result != GL_TRUE)
		{
			shader->program = 0;
			shader->custom = false;
			Shader_CompileError("Shader_Compile: Error linking shader program %d\n", i);
			continue;
		}

#ifdef HAVE_GLES2
		memset(shader->projMatrix, 0x00, sizeof(fmatrix4_t));
		memset(shader->viewMatrix, 0x00, sizeof(fmatrix4_t));
		memset(shader->modelMatrix, 0x00, sizeof(fmatrix4_t));
#endif

		// 09072020 / 13062019
#define GETUNI(uniform) pglGetUniformLocation(shader->program, uniform);

#ifdef HAVE_GLES2
		// transform
		shader->uniforms[gluniform_model]       = GETUNI(GLSL_UNIFORM_MODEL);
		shader->uniforms[gluniform_view]        = GETUNI(GLSL_UNIFORM_VIEW);
		shader->uniforms[gluniform_projection]  = GETUNI(GLSL_UNIFORM_PROJECTION);

		// samplers
		shader->uniforms[gluniform_startscreen] = GETUNI(GLSL_UNIFORM_STARTSCREEN);
		shader->uniforms[gluniform_endscreen]   = GETUNI(GLSL_UNIFORM_ENDSCREEN);
		shader->uniforms[gluniform_fademask]    = GETUNI(GLSL_UNIFORM_FADEMASK);

		// misc.
		shader->uniforms[gluniform_isfadingin]  = GETUNI(GLSL_UNIFORM_ISFADINGIN);
		shader->uniforms[gluniform_istowhite]   = GETUNI(GLSL_UNIFORM_ISTOWHITE);
#endif

		// lighting
		shader->uniforms[gluniform_poly_color]  = GETUNI(GLSL_UNIFORM_POLYCOLOR);
		shader->uniforms[gluniform_tint_color]  = GETUNI(GLSL_UNIFORM_TINTCOLOR);
		shader->uniforms[gluniform_fade_color]  = GETUNI(GLSL_UNIFORM_FADECOLOR);
		shader->uniforms[gluniform_lighting]    = GETUNI(GLSL_UNIFORM_LIGHTING);
		shader->uniforms[gluniform_fade_start]  = GETUNI(GLSL_UNIFORM_FADESTART);
		shader->uniforms[gluniform_fade_end]    = GETUNI(GLSL_UNIFORM_FADEEND);

		// misc.
		shader->uniforms[gluniform_leveltime]   = GETUNI(GLSL_UNIFORM_LEVELTIME);

#undef GETUNI

		// 27072020
#ifdef GLSL_USE_ATTRIBUTE_QUALIFIER
#define GETATTRIB(attribute) pglGetAttribLocation(shader->program, attribute);

		shader->attributes[glattribute_position]     = GETATTRIB(GLSL_ATTRIBUTE_POSITION);
		shader->attributes[glattribute_texcoord]     = GETATTRIB(GLSL_ATTRIBUTE_TEXCOORD);
		shader->attributes[glattribute_normal]       = GETATTRIB(GLSL_ATTRIBUTE_NORMAL);
		shader->attributes[glattribute_colors]       = GETATTRIB(GLSL_ATTRIBUTE_COLORS);
		shader->attributes[glattribute_fadetexcoord] = GETATTRIB(GLSL_ATTRIBUTE_FADETEX);

#undef GETATTRIB
#endif
	}

#ifdef HAVE_GLES2
	pglUseProgram(shader_base->program);
#endif

	return true;
#else
	return false;
#endif
}

void *Shader_SetColors(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
#ifdef GL_SHADERS
	if (gl_shadersenabled && pglUseProgram)
	{
		gl_shaderprogram_t *shader = &gl_shaderprograms[gl_currentshaderprogram];
		if (shader->program)
		{
			if (gl_shaderprogramchanged)
			{
				pglUseProgram(gl_shaderprograms[gl_currentshaderprogram].program);
				gl_shaderprogramchanged = false;
			}
			Shader_SetUniforms(Surface, poly, tint, fade);
			return shader;
		}
		else
			pglUseProgram(0);
	}
#else
	(void)Surface;
	(void)poly;
	(void)tint;
	(void)fade;
#endif
	return NULL;
}

#ifdef HAVE_GLES2
void Shader_SetTransform(void)
{
	if (shader_current == NULL)
		return;

	if (memcmp(projMatrix, shader_current->projMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader_current->projMatrix, projMatrix, sizeof(fmatrix4_t));
		pglUniformMatrix4fv(shader_current->uniforms[gluniform_projection], 1, GL_FALSE, (float *)projMatrix);
	}

	if (memcmp(viewMatrix, shader_current->viewMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader_current->viewMatrix, viewMatrix, sizeof(fmatrix4_t));
		pglUniformMatrix4fv(shader_current->uniforms[gluniform_view], 1, GL_FALSE, (float *)viewMatrix);
	}

	if (memcmp(modelMatrix, shader_current->modelMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader_current->modelMatrix, modelMatrix, sizeof(fmatrix4_t));
		pglUniformMatrix4fv(shader_current->uniforms[gluniform_model], 1, GL_FALSE, (float *)modelMatrix);
	}
}
#endif

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
#ifdef HAVE_GLES2
	gl_shaderprogram_t *shader = shader_current;
#else
	gl_shaderprogram_t *shader = &gl_shaderprograms[gl_currentshaderprogram];
#endif

#ifdef GL_SHADERS
	if (!gl_shadersenabled)
		return;
#endif

#if defined(GL_SHADERS) || defined(HAVE_GLES2)
	if (shader == NULL)
		return;

	if (!shader->program)
		return;

	#define UNIFORM_1(uniform, a, function) \
		if (uniform != -1) \
			function (uniform, a);

	#define UNIFORM_2(uniform, a, b, function) \
		if (uniform != -1) \
			function (uniform, a, b);

	#define UNIFORM_3(uniform, a, b, c, function) \
		if (uniform != -1) \
			function (uniform, a, b, c);

	#define UNIFORM_4(uniform, a, b, c, d, function) \
		if (uniform != -1) \
			function (uniform, a, b, c, d);

#ifdef HAVE_GLES2
	if (poly)
#endif
		UNIFORM_4(shader->uniforms[gluniform_poly_color], poly->red, poly->green, poly->blue, poly->alpha, pglUniform4f);

#ifdef HAVE_GLES2
	if (tint)
#endif
		UNIFORM_4(shader->uniforms[gluniform_tint_color], tint->red, tint->green, tint->blue, tint->alpha, pglUniform4f);

#ifdef HAVE_GLES2
	if (fade)
#endif
		UNIFORM_4(shader->uniforms[gluniform_fade_color], fade->red, fade->green, fade->blue, fade->alpha, pglUniform4f);

	if (Surface != NULL)
	{
		UNIFORM_1(shader->uniforms[gluniform_lighting], Surface->LightInfo.light_level, pglUniform1f);
		UNIFORM_1(shader->uniforms[gluniform_fade_start], Surface->LightInfo.fade_start, pglUniform1f);
		UNIFORM_1(shader->uniforms[gluniform_fade_end], Surface->LightInfo.fade_end, pglUniform1f);
	}

	UNIFORM_1(shader->uniforms[gluniform_leveltime], ((float)shader_leveltime) / TICRATE, pglUniform1f);

	#undef UNIFORM_1
	#undef UNIFORM_2
	#undef UNIFORM_3
	#undef UNIFORM_4
#else // defined(GL_SHADERS) || defined(HAVE_GLES2)
	(void)Surface;
	(void)poly;
	(void)tint;
	(void)fade;
#endif
}

void Shader_SetSampler(gl_shaderprogram_t *shader, gluniform_t uniform, GLint value)
{
	if (shader == NULL)
		return;

	if (shader->uniforms[uniform] != -1)
		pglUniform1i(shader->uniforms[uniform], value);
}
