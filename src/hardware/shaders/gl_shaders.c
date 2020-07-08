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

boolean gl_allowshaders = false;
boolean gl_shadersenabled = false;

#ifdef GL_SHADERS
typedef GLuint 	(APIENTRY *PFNglCreateShader)		(GLenum);
typedef void 	(APIENTRY *PFNglShaderSource)		(GLuint, GLsizei, const GLchar**, GLint*);
typedef void 	(APIENTRY *PFNglCompileShader)		(GLuint);
typedef void 	(APIENTRY *PFNglGetShaderiv)		(GLuint, GLenum, GLint*);
typedef void 	(APIENTRY *PFNglGetShaderInfoLog)	(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void 	(APIENTRY *PFNglDeleteShader)		(GLuint);
typedef GLuint 	(APIENTRY *PFNglCreateProgram)		(void);
typedef void 	(APIENTRY *PFNglAttachShader)		(GLuint, GLuint);
typedef void 	(APIENTRY *PFNglLinkProgram)		(GLuint);
typedef void 	(APIENTRY *PFNglGetProgramiv)		(GLuint, GLenum, GLint*);
typedef void 	(APIENTRY *PFNglUseProgram)			(GLuint);
typedef void 	(APIENTRY *PFNglUniform1i)			(GLint, GLint);
typedef void 	(APIENTRY *PFNglUniform1f)			(GLint, GLfloat);
typedef void 	(APIENTRY *PFNglUniform2f)			(GLint, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform3f)			(GLint, GLfloat, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform4f)			(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform1fv)			(GLint, GLsizei, const GLfloat*);
typedef void 	(APIENTRY *PFNglUniform2fv)			(GLint, GLsizei, const GLfloat*);
typedef void 	(APIENTRY *PFNglUniform3fv)			(GLint, GLsizei, const GLfloat*);
typedef GLint 	(APIENTRY *PFNglGetUniformLocation)	(GLuint, const GLchar*);

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
static PFNglGetUniformLocation pglGetUniformLocation;

#define MAXSHADERS 16
#define MAXSHADERPROGRAMS 16

// 18032019
static char *gl_customvertexshaders[MAXSHADERS];
static char *gl_customfragmentshaders[MAXSHADERS];
static GLuint gl_currentshaderprogram = 0;
static boolean gl_shaderprogramchanged = true;

// 13062019
typedef enum
{
	// lighting
	gluniform_poly_color,
	gluniform_tint_color,
	gluniform_fade_color,
	gluniform_lighting,
	gluniform_fade_start,
	gluniform_fade_end,

	// misc. (custom shaders)
	gluniform_leveltime,

	gluniform_max,
} gluniform_t;

typedef struct gl_shaderprogram_s
{
	GLuint program;
	boolean custom;
	GLint uniforms[gluniform_max+1];
} gl_shaderprogram_t;
static gl_shaderprogram_t gl_shaderprograms[MAXSHADERPROGRAMS];

// Shader info
static INT32 shader_leveltime = 0;

// ========================
//  Fragment shader macros
// ========================

//
// GLSL Software fragment shader
//

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
		"float color_bright = sqrt((base_color.r * base_color.r) + (base_color.g * base_color.g) + (base_color.b * base_color.b));\n" \
		"float strength = sqrt(9.0 * tint_color.a);\n" \
		"final_color.r = clamp((color_bright * (tint_color.r * strength)) + (base_color.r * (1.0 - strength)), 0.0, 1.0);\n" \
		"final_color.g = clamp((color_bright * (tint_color.g * strength)) + (base_color.g * (1.0 - strength)), 0.0, 1.0);\n" \
		"final_color.b = clamp((color_bright * (tint_color.b * strength)) + (base_color.b * (1.0 - strength)), 0.0, 1.0);\n" \
	"}\n"

#define GLSL_SOFTWARE_FADE_EQUATION \
	"float darkness = R_DoomLightingEquation(lighting);\n" \
	"if (fade_start != 0.0 || fade_end != 31.0) {\n" \
		"float fs = fade_start / 31.0;\n" \
		"float fe = fade_end / 31.0;\n" \
		"float fd = fe - fs;\n" \
		"darkness = clamp((darkness - fs) * (1.0 / fd), 0.0, 1.0);\n" \
	"}\n" \
	"final_color = mix(final_color, fade_color, darkness);\n"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 base_color = texel * poly_color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"final_color.a = texel.a * poly_color.a;\n" \
		"gl_FragColor = final_color;\n" \
	"}\0"

//
// Water surface shader
//
// Mostly guesstimated, rather than the rest being built off Software science.
// Still needs to distort things underneath/around the water...
//

#define GLSL_WATER_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	"uniform float leveltime;\n" \
	"const float freq = 0.025;\n" \
	"const float amp = 0.025;\n" \
	"const float speed = 2.0;\n" \
	"const float pi = 3.14159;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"float z = (gl_FragCoord.z / gl_FragCoord.w) / 2.0;\n" \
		"float a = -pi * (z * freq) + (leveltime * speed);\n" \
		"float sdistort = sin(a) * amp;\n" \
		"float cdistort = cos(a) * amp;\n" \
		"vec4 texel = texture2D(tex, vec2(gl_TexCoord[0].s - sdistort, gl_TexCoord[0].t - cdistort));\n" \
		"vec4 base_color = texel * poly_color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"final_color.a = texel.a * poly_color.a;\n" \
		"gl_FragColor = final_color;\n" \
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
		"vec4 base_color = gl_Color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"gl_FragColor = final_color;\n" \
	"}\0"

//
// GLSL generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * poly_color;\n" \
	"}\0"

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
	"uniform sampler2D tex;\n"
	"void main(void) {\n"
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"}\0",

	NULL,
};

// ======================
//  Vertex shader macros
// ======================

//
// GLSL generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = gl_Color;\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

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
	pglGetUniformLocation = GetGLFunc("glGetUniformLocation");
#endif
}

// -----------------+
// GL_MSG_Error     : Raises an error.
//                  :
// Returns          :
// -----------------+

static void GL_MSG_Error(const char *format, ...)
{
	char str[4096] = "";
	va_list arglist;

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

#ifdef HAVE_SDL
	CONS_Alert(CONS_ERROR, "%s", str);
#endif
#ifdef DEBUG_TO_FILE
	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");
	fwrite(str, strlen(str), 1, gllogstream);
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
	Shader_Kill();
	return Shader_Compile();
#endif
}

void Shader_Set(int shader)
{
#ifdef GL_SHADERS
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
#ifdef GL_SHADERS
	gl_shadersenabled = false;
	gl_currentshaderprogram = 0;
	if (!pglUseProgram) return;
	pglUseProgram(0);
#endif
}

void Shader_Kill(void)
{
	// unused.........................
}

boolean Shader_Compile(void)
{
#ifdef GL_SHADERS
	GLuint gl_vertShader, gl_fragShader;
	GLint i, result;

	if (!pglUseProgram) return false;

	gl_customvertexshaders[0] = NULL;
	gl_customfragmentshaders[0] = NULL;

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
			GL_MSG_Error("LoadShaders: Error creating vertex shader %d\n", i);
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

			GL_MSG_Error("LoadShaders: Error compiling vertex shader %d\n%s", i, infoLog);
			continue;
		}

		//
		// Load and compile fragment shader
		//
		gl_fragShader = pglCreateShader(GL_FRAGMENT_SHADER);
		if (!gl_fragShader)
		{
			GL_MSG_Error("LoadShaders: Error creating fragment shader %d\n", i);
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

			GL_MSG_Error("LoadShaders: Error compiling fragment shader %d\n%s", i, infoLog);
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
			GL_MSG_Error("LoadShaders: Error linking shader program %d\n", i);
			continue;
		}

		// 13062019
#define GETUNI(uniform) pglGetUniformLocation(shader->program, uniform);

		// lighting
		shader->uniforms[gluniform_poly_color] = GETUNI("poly_color");
		shader->uniforms[gluniform_tint_color] = GETUNI("tint_color");
		shader->uniforms[gluniform_fade_color] = GETUNI("fade_color");
		shader->uniforms[gluniform_lighting] = GETUNI("lighting");
		shader->uniforms[gluniform_fade_start] = GETUNI("fade_start");
		shader->uniforms[gluniform_fade_end] = GETUNI("fade_end");

		// misc. (custom shaders)
		shader->uniforms[gluniform_leveltime] = GETUNI("leveltime");

#undef GETUNI
	}
#endif
	return true;
}

void *Shader_Load(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
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

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
#ifdef GL_SHADERS
	if (gl_shadersenabled)
	{
		gl_shaderprogram_t *shader = &gl_shaderprograms[gl_currentshaderprogram];
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

		// polygon
		UNIFORM_4(shader->uniforms[gluniform_poly_color], poly->red, poly->green, poly->blue, poly->alpha, pglUniform4f);
		UNIFORM_4(shader->uniforms[gluniform_tint_color], tint->red, tint->green, tint->blue, tint->alpha, pglUniform4f);
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
	}
#else
	(void)Surface;
	(void)poly;
	(void)tint;
	(void)fade;
#endif
}
