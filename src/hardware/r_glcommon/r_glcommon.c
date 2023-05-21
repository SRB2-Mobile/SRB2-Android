// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2021 by Sonic Team Junior.
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_glcommon.c
/// \brief Common OpenGL functions shared by OpenGL backends

#include "r_glcommon.h"

#include "../../doomdata.h"
#include "../../doomtype.h"
#include "../../doomdef.h"
#include "../../console.h"

#ifdef GL_SHADERS
#include "../shaders/gl_shaders.h"
#endif

#include <stdarg.h>

const GLubyte *gl_version = NULL;
const GLubyte *gl_renderer = NULL;
const GLubyte *gl_extensions = NULL;

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

RGBA_t *TextureBuffer = NULL;
static size_t TextureBufferSize = 0;

RGBA_t  myPaletteData[256];
GLint   screen_width    = 0;
GLint   screen_height   = 0;
GLbyte  screen_depth    = 0;
GLint   textureformatGL = 0;
GLint maximumAnisotropy = 0;

GLboolean MipmapEnabled = GL_FALSE;
GLboolean MipmapSupported = GL_FALSE;
GLint min_filter = GL_LINEAR;
GLint mag_filter = GL_LINEAR;
GLint anisotropic_filter = 0;

boolean alpha_test = false;
float alpha_threshold = 0.0f;

float near_clipping_plane = NZCLIP_PLANE;

// Linked list of all textures.
FTextureInfo *TexCacheTail = NULL;
FTextureInfo *TexCacheHead = NULL;

GLuint      tex_downloaded  = 0;
GLfloat     fov             = 90.0f;
FBITFIELD   CurrentPolyFlags;

GLuint screentexture = 0;
GLuint startScreenWipe = 0;
GLuint endScreenWipe = 0;
GLuint finalScreenTexture = 0;

static GLuint blank_texture_num = 0;

#ifdef HAVE_GL_FRAMEBUFFER
GLuint FramebufferObject, FramebufferTexture;
GLuint RenderbufferObject, RenderbufferDepthBits;
GLboolean FramebufferEnabled = GL_FALSE, RenderToFramebuffer = GL_FALSE;

static GLuint LastRenderbufferDepthBits;

GLenum RenderbufferFormats[NumRenderbufferFormats] =
{
	GL_DEPTH_COMPONENT,
	GL_DEPTH_COMPONENT16,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH_COMPONENT32,
	GL_DEPTH_COMPONENT32F
};
#endif

// Linked list of all models.
static GLModelList *ModelListTail = NULL;
static GLModelList *ModelListHead = NULL;

boolean model_lighting = false;

// ==========================================================================
//                                                                 EXTENSIONS
// ==========================================================================

boolean GLExtension_multitexture;
boolean GLExtension_vertex_buffer_object;
boolean GLExtension_texture_filter_anisotropic;
boolean GLExtension_vertex_program;
boolean GLExtension_fragment_program;
#ifdef HAVE_GL_FRAMEBUFFER
boolean GLExtension_framebuffer_object;
#endif
boolean GLExtension_shaders; // Not an extension on its own, but it is set if multiple extensions are available.

static FExtensionList const ExtensionList[] = {
	{"GL_ARB_multitexture", &GLExtension_multitexture},

#ifndef HAVE_GLES2
	{"GL_ARB_vertex_buffer_object", &GLExtension_vertex_buffer_object},
#endif

	{"GL_ARB_texture_filter_anisotropic", &GLExtension_texture_filter_anisotropic},
	{"GL_EXT_texture_filter_anisotropic", &GLExtension_texture_filter_anisotropic},

#ifndef HAVE_GLES2
	{"GL_ARB_vertex_program", &GLExtension_vertex_program},
	{"GL_ARB_fragment_program", &GLExtension_fragment_program},
#endif

#ifdef HAVE_GL_FRAMEBUFFER
	{"GL_ARB_framebuffer_object", &GLExtension_framebuffer_object},
	{"GL_OES_framebuffer_object", &GLExtension_framebuffer_object},
#endif

	{NULL, NULL}
};

static void PrintExtensions(const GLubyte *extensions);

// ==========================================================================
//                                                           OPENGL FUNCTIONS
// ==========================================================================

#ifndef STATIC_OPENGL
/* Miscellaneous */
PFNglClear pglClear;
PFNglGetFloatv pglGetFloatv;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;
PFNglGetError pglGetError;
PFNglClearColor pglClearColor;
PFNglColorMask pglColorMask;
PFNglAlphaFunc pglAlphaFunc;
PFNglBlendFunc pglBlendFunc;
PFNglCullFace pglCullFace;
PFNglPolygonOffset pglPolygonOffset;
PFNglEnable pglEnable;
PFNglDisable pglDisable;

/* Depth buffer */
PFNglDepthFunc pglDepthFunc;
PFNglDepthMask pglDepthMask;

/* Transformation */
PFNglViewport pglViewport;

/* Raster functions */
PFNglPixelStorei pglPixelStorei;
PFNglReadPixels pglReadPixels;

/* Texture mapping */
PFNglTexParameteri pglTexParameteri;
PFNglTexImage2D pglTexImage2D;
PFNglTexSubImage2D pglTexSubImage2D;

/* Drawing functions */
PFNglDrawArrays pglDrawArrays;
PFNglDrawElements pglDrawElements;

/* Texture objects */
PFNglGenTextures pglGenTextures;
PFNglDeleteTextures pglDeleteTextures;
PFNglBindTexture pglBindTexture;

/* Texture mapping */
PFNglCopyTexImage2D pglCopyTexImage2D;
PFNglCopyTexSubImage2D pglCopyTexSubImage2D;
#endif

//
// Multitexturing
//

#ifndef STATIC_OPENGL
PFNglActiveTexture pglActiveTexture;
PFNglClientActiveTexture pglClientActiveTexture;
#endif

//
// Mipmapping
//

#ifdef HAVE_GLES
PFNglGenerateMipmap pglGenerateMipmap;
#endif

//
// Depth functions
//

#ifndef HAVE_GLES
	PFNglClearDepth pglClearDepth;
	PFNglDepthRange pglDepthRange;
#else
	PFNglClearDepthf pglClearDepthf;
	PFNglDepthRangef pglDepthRangef;
#endif

//
// Legacy functions
//

#ifndef HAVE_GLES2
#ifndef STATIC_OPENGL
/* Transformation */
PFNglMatrixMode pglMatrixMode;
PFNglViewport pglViewport;
PFNglPushMatrix pglPushMatrix;
PFNglPopMatrix pglPopMatrix;
PFNglLoadIdentity pglLoadIdentity;
PFNglMultMatrixf pglMultMatrixf;
PFNglRotatef pglRotatef;
PFNglScalef pglScalef;
PFNglTranslatef pglTranslatef;

/* Drawing functions */
PFNglVertexPointer pglVertexPointer;
PFNglNormalPointer pglNormalPointer;
PFNglTexCoordPointer pglTexCoordPointer;
PFNglColorPointer pglColorPointer;
PFNglEnableClientState pglEnableClientState;
PFNglDisableClientState pglDisableClientState;

/* Lighting */
PFNglShadeModel pglShadeModel;
PFNglLightfv pglLightfv;
PFNglLightModelfv pglLightModelfv;
PFNglMaterialfv pglMaterialfv;

/* Texture mapping */
PFNglTexEnvi pglTexEnvi;
#endif
#endif // HAVE_GLES2

// Color
#ifdef HAVE_GLES
PFNglColor4f pglColor4f;
#else
PFNglColor4ubv pglColor4ubv;
#endif

/* 1.5 functions for buffers */
PFNglGenBuffers pglGenBuffers;
PFNglBindBuffer pglBindBuffer;
PFNglBufferData pglBufferData;
PFNglDeleteBuffers pglDeleteBuffers;

/* 2.0 functions */
PFNglBlendEquation pglBlendEquation;

#ifdef HAVE_GL_FRAMEBUFFER
/* 3.0 functions for framebuffers and renderbuffers */
PFNglGenFramebuffers pglGenFramebuffers;
PFNglBindFramebuffer pglBindFramebuffer;
PFNglDeleteFramebuffers pglDeleteFramebuffers;
PFNglFramebufferTexture2D pglFramebufferTexture2D;
PFNglCheckFramebufferStatus pglCheckFramebufferStatus;
PFNglGenRenderbuffers pglGenRenderbuffers;
PFNglBindRenderbuffer pglBindRenderbuffer;
PFNglDeleteRenderbuffers pglDeleteRenderbuffers;
PFNglRenderbufferStorage pglRenderbufferStorage;
PFNglFramebufferRenderbuffer pglFramebufferRenderbuffer;
#endif

static const char *GetGLError(GLenum error)
{
	if (error == GL_NO_ERROR)
		return "GL_NO_ERROR";

	switch (error)
	{
		case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
#ifdef HAVE_GL_FRAMEBUFFER
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
		default:                               return "unknown error";
	}
}

struct GLError
{
	char *func;
	char *file;
	int line;
	GLenum error;
};

static struct GLError *gl_past_errors_list = NULL;
static size_t gl_past_errors_list_size = 0;

static boolean FindPastError(const char *func, const char *file, int line, GLenum error)
{
	size_t i = 0;

	if (gl_past_errors_list)
	{
		for (; i < gl_past_errors_list_size; i++)
		{
			if (!gl_past_errors_list[i].func || !gl_past_errors_list[i].file)
				break;
			if (strcmp(gl_past_errors_list[i].func, func))
				break;
			if (strcmp(gl_past_errors_list[i].file, file))
				break;
			if (gl_past_errors_list[i].line != line)
				break;
			if (gl_past_errors_list[i].error != error)
				break;
			return true;
		}
	}

	gl_past_errors_list_size++;
	gl_past_errors_list = realloc(gl_past_errors_list, sizeof(struct GLError) * gl_past_errors_list_size);

	if (!gl_past_errors_list)
	{
		i = 0;
		gl_past_errors_list_size = 1;
		gl_past_errors_list = malloc(sizeof(struct GLError));
		if (!gl_past_errors_list)
			return false;
	}

	struct GLError *err = &gl_past_errors_list[i];
	err->func = malloc(strlen(func) + 1);
	err->file = malloc(strlen(file) + 1);
	err->line = line;
	err->error = error;

	if (err->func)
		memcpy(err->func, func, strlen(func) + 1);
	if (err->file)
		memcpy(err->file, file, strlen(file) + 1);

	return false;
}

void GLBackend_CheckError(const char *func, const char *file, int line)
{
	GLenum error = pglGetError();
	while (error != GL_NO_ERROR)
	{
		if (!FindPastError(func, file, line, error))
			GL_DBG_Printf("%s (%s, line %d): %s\n", func, file, line, GetGLError(error));
		error = pglGetError();
	}
}

#define CHECK_GL() CHECK_GL_ERROR("r_glcommon.c")

void gl_Clear(GLbitfield mask)
{
	if (pglClear)
	{
		pglClear(mask);
		CHECK_GL();
	}
}
void gl_GetFloatv(GLenum pname, GLfloat *params)
{
	if (pglGetFloatv)
	{
		pglGetFloatv(pname, params);
		CHECK_GL();
	}
}
void gl_GetIntegerv(GLenum pname, GLint *params)
{
	if (pglGetIntegerv)
	{
		pglGetIntegerv(pname, params);
		CHECK_GL();
	}
}
const GLubyte *gl_GetString(GLenum name)
{
	const GLubyte *result = NULL;
	if (pglGetString)
	{
		result = pglGetString(name);
		CHECK_GL();
	}
	return result;
}
void gl_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	if (pglClearColor)
	{
		pglClearColor(red, green, blue, alpha);
		CHECK_GL();
	}
}
void gl_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	if (pglColorMask)
	{
		pglColorMask(red, green, blue, alpha);
		CHECK_GL();
	}
}
void gl_AlphaFunc(GLenum func, GLclampf ref)
{
	if (pglAlphaFunc)
	{
		pglAlphaFunc(func, ref);
		CHECK_GL();
	}
}
void gl_BlendFunc(GLenum sfactor, GLenum dfactor)
{
	if (pglBlendFunc)
	{
		pglBlendFunc(sfactor, dfactor);
		CHECK_GL();
	}
}
void gl_CullFace(GLenum mode)
{
	if (pglCullFace)
	{
		pglCullFace(mode);
		CHECK_GL();
	}
}
void gl_PolygonOffset(GLfloat factor, GLfloat units)
{
	if (pglPolygonOffset)
	{
		pglPolygonOffset(factor, units);
		CHECK_GL();
	}
}
void gl_Enable(GLenum cap)
{
	if (pglEnable)
	{
		pglEnable(cap);
		CHECK_GL();
	}
}
void gl_Disable(GLenum cap)
{
	if (pglDisable)
	{
		pglDisable(cap);
		CHECK_GL();
	}
}

/* Depth buffer */
void gl_DepthFunc(GLenum func)
{
	if (pglDepthFunc)
	{
		pglDepthFunc(func);
		CHECK_GL();
	}
}
void gl_DepthMask(GLboolean flag)
{
	if (pglDepthMask)
	{
		pglDepthMask(flag);
		CHECK_GL();
	}
}

/* Transformation */
void gl_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (pglViewport)
	{
		pglViewport(x, y, width, height);
		CHECK_GL();
	}
}

/* Raster functions */
void gl_PixelStorei(GLenum pname, GLint param)
{
	if (pglPixelStorei)
	{
		pglPixelStorei(pname, param);
		CHECK_GL();
	}
}
void gl_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	if (pglReadPixels)
	{
		pglReadPixels(x, y, width, height, format, type, pixels);
		CHECK_GL();
	}
}

/* Texture mapping */
void gl_TexParameteri(GLenum target, GLenum pname, GLint param)
{
	if (pglTexParameteri)
	{
		pglTexParameteri(target, pname, param);
		CHECK_GL();
	}
}
void gl_TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	if (pglTexImage2D)
	{
		pglTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
		CHECK_GL();
	}
}
void gl_TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	if (pglTexSubImage2D)
	{
		pglTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
		CHECK_GL();
	}
}

/* Drawing functions */
void gl_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (pglDrawArrays)
	{
		pglDrawArrays(mode, first, count);
		CHECK_GL();
	}
}
void gl_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	if (pglDrawElements)
	{
		pglDrawElements(mode, count, type, indices);
		CHECK_GL();
	}
}

/* Texture objects */
void gl_GenTextures(GLsizei n, const GLuint *textures)
{
	if (pglGenTextures)
	{
		pglGenTextures(n, textures);
		CHECK_GL();
	}
}
void gl_DeleteTextures(GLsizei n, const GLuint *textures)
{
	if (pglDeleteTextures)
	{
		pglDeleteTextures(n, textures);
		CHECK_GL();
	}
}
void gl_BindTexture(GLenum target, GLuint texture)
{
	if (pglBindTexture)
	{
		pglBindTexture(target, texture);
		CHECK_GL();
	}
}

/* Texture mapping */
void gl_CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	if (pglCopyTexImage2D)
	{
		pglCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
		CHECK_GL();
	}
}
void gl_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (pglCopyTexSubImage2D)
	{
		pglCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
		CHECK_GL();
	}
}

//
// Multitexturing
//

void gl_ActiveTexture(GLenum texture)
{
	if (pglActiveTexture)
	{
		pglActiveTexture(texture);
		CHECK_GL();
	}
}
void gl_ClientActiveTexture(GLenum texture)
{
	if (pglClientActiveTexture)
	{
		pglClientActiveTexture(texture);
		CHECK_GL();
	}
}

//
// Mipmapping
//

#ifdef HAVE_GLES
void gl_GenerateMipmap(GLenum target)
{
	if (pglGenerateMipmap)
	{
		pglGenerateMipmap(target);
		CHECK_GL();
	}
}
#endif

//
// Depth functions
//

#ifndef HAVE_GLES
void gl_ClearDepth(GLclampd depth)
{
	if (pglClearDepth)
	{
		pglClearDepth(depth);
		CHECK_GL();
	}
}
void gl_DepthRange(GLclampd near_val, GLclampd far_val)
{
	if (pglDepthRange)
	{
		pglDepthRange(near_val, far_val);
		CHECK_GL();
	}
}
#else
void gl_ClearDepthf(GLclampf depth)
{
	if (pglClearDepthf)
	{
		pglClearDepthf(depth);
		CHECK_GL();
	}
}
void gl_DepthRangef(GLclampf near_val, GLclampf far_val)
{
	if (pglDepthRangef)
	{
		pglDepthRangef(near_val, far_val);
		CHECK_GL();
	}
}
#endif

//
// Legacy functions
//

#ifndef HAVE_GLES2
void gl_MatrixMode(GLenum mode)
{
	if (pglMatrixMode)
	{
		pglMatrixMode(mode);
		CHECK_GL();
	}
}
void gl_PushMatrix(void)
{
	if (pglPushMatrix)
	{
		pglPushMatrix();
		CHECK_GL();
	}
}
void gl_PopMatrix(void)
{
	if (pglPopMatrix)
	{
		pglPopMatrix();
		CHECK_GL();
	}
}
void gl_LoadIdentity(void)
{
	if (pglLoadIdentity)
	{
		pglLoadIdentity();
		CHECK_GL();
	}
}
void gl_MultMatrixf(const GLfloat *m)
{
	if (pglMultMatrixf)
	{
		pglMultMatrixf(m);
		CHECK_GL();
	}
}
void gl_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	if (pglRotatef)
	{
		pglRotatef(angle, x, y, z);
		CHECK_GL();
	}
}
void gl_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
	if (pglScalef)
	{
		pglScalef(x, y, z);
		CHECK_GL();
	}
}
void gl_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
	if (pglTranslatef)
	{
		pglTranslatef(x, y, z);
		CHECK_GL();
	}
}

/* Drawing Functions */
void gl_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (pglVertexPointer)
	{
		pglVertexPointer(size, type, stride, pointer);
		CHECK_GL();
	}
}
void gl_NormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (pglNormalPointer)
	{
		pglNormalPointer(type, stride, pointer);
		CHECK_GL();
	}
}
void gl_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (pglTexCoordPointer)
	{
		pglTexCoordPointer(size, type, stride, pointer);
		CHECK_GL();
	}
}
void gl_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (pglColorPointer)
	{
		pglColorPointer(size, type, stride, pointer);
		CHECK_GL();
	}
}
void gl_EnableClientState(GLenum cap)
{
	if (pglEnableClientState)
	{
		pglEnableClientState(cap);
		CHECK_GL();
	}
}
void gl_DisableClientState(GLenum cap)
{
	if (pglDisableClientState)
	{
		pglDisableClientState(cap);
		CHECK_GL();
	}
}

/* Lighting */
void gl_ShadeModel(GLenum mode)
{
	if (pglShadeModel)
	{
		pglShadeModel(mode);
		CHECK_GL();
	}
}
void gl_Lightfv(GLenum light, GLenum pname, GLfloat *params)
{
	if (pglLightfv)
	{
		pglLightfv(light, pname, params);
		CHECK_GL();
	}
}
void gl_LightModelfv(GLenum pname, GLfloat *params)
{
	if (pglLightModelfv)
	{
		pglLightModelfv(pname, params);
		CHECK_GL();
	}
}
void gl_Materialfv(GLint face, GLenum pname, GLfloat *params)
{
	if (pglMaterialfv)
	{
		pglMaterialfv(face, pname, params);
		CHECK_GL();
	}
}

/* Texture mapping */
void gl_TexEnvi(GLenum target, GLenum pname, GLint param)
{
	if (pglTexEnvi)
	{
		pglTexEnvi(target, pname, param);
		CHECK_GL();
	}
}
#endif // HAVE_GLES2

// Color
#ifdef HAVE_GLES
void gl_Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if (pglColor4f)
	{
		pglColor4f(red, green, blue, alpha);
		CHECK_GL();
	}
}
#else
void gl_Color4ubv(const GLubyte *v)
{
	if (pglColor4ubv)
	{
		pglColor4ubv(v);
		CHECK_GL();
	}
}
#endif

/* 1.5 functions for buffers */
void gl_GenBuffers(GLsizei n, GLuint *buffers)
{
	if (pglGenBuffers)
	{
		pglGenBuffers(n, buffers);
		CHECK_GL();
	}
}
void gl_BindBuffer(GLenum target, GLuint buffer)
{
	if (pglBindBuffer)
	{
		pglBindBuffer(target, buffer);
		CHECK_GL();
	}
}
void gl_BufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage)
{
	if (pglBufferData)
	{
		pglBufferData(target, size, data, usage);
		CHECK_GL();
	}
}
void gl_DeleteBuffers(GLsizei n, const GLuint *buffers)
{
	if (pglDeleteBuffers)
	{
		pglDeleteBuffers(n, buffers);
		CHECK_GL();
	}
}

/* 2.0 functions */
void gl_BlendEquation(GLenum mode)
{
	if (pglBlendEquation)
	{
		pglBlendEquation(mode);
		CHECK_GL();
	}
}

#ifdef HAVE_GL_FRAMEBUFFER
/* 3.0 functions for framebuffers and renderbuffers */
void gl_GenFramebuffers(GLsizei n, GLuint *ids)
{
	if (pglGenFramebuffers)
	{
		pglGenFramebuffers(n, ids);
		CHECK_GL();
	}
}
void gl_BindFramebuffer(GLenum target, GLuint framebuffer)
{
	if (pglBindFramebuffer)
	{
		pglBindFramebuffer(target, framebuffer);
		CHECK_GL();
	}
}
void gl_DeleteFramebuffers(GLsizei n, GLuint *ids)
{
	if (pglDeleteFramebuffers)
	{
		pglDeleteFramebuffers(n, ids);
		CHECK_GL();
	}
}
void gl_FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	if (pglFramebufferTexture2D)
	{
		pglFramebufferTexture2D(target, attachment, textarget, texture, level);
		CHECK_GL();
	}
}
GLenum gl_CheckFramebufferStatus(GLenum target)
{
	GLenum result = 0;
	if (pglCheckFramebufferStatus)
	{
		result = pglCheckFramebufferStatus(target);
		CHECK_GL();
	}
	return result;
}
void gl_GenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
	if (pglGenRenderbuffers)
	{
		pglGenRenderbuffers(n, renderbuffers);
		CHECK_GL();
	}
}
void gl_BindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	if (pglBindRenderbuffer)
	{
		pglBindRenderbuffer(target, renderbuffer);
		CHECK_GL();
	}
}
void gl_DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
	if (pglDeleteRenderbuffers)
	{
		pglDeleteRenderbuffers(n, renderbuffers);
		CHECK_GL();
	}
}
void gl_RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	if (pglRenderbufferStorage)
	{
		pglRenderbufferStorage(target, internalformat, width, height);
		CHECK_GL();
	}
}
void gl_FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLenum renderbuffer)
{
	if (pglFramebufferRenderbuffer)
	{
		pglFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
		CHECK_GL();
	}
}
#endif

boolean GLBackend_LoadCommonFunctions(void)
{
	GETOPENGLFUNC(ClearColor)

	GETOPENGLFUNC(Clear)
	GETOPENGLFUNC(ColorMask)
	GETOPENGLFUNC(AlphaFunc)
	GETOPENGLFUNC(BlendFunc)
	GETOPENGLFUNC(CullFace)
	GETOPENGLFUNC(PolygonOffset)
	GETOPENGLFUNC(Enable)
	GETOPENGLFUNC(Disable)
	GETOPENGLFUNC(GetFloatv)
	GETOPENGLFUNC(GetIntegerv)
	GETOPENGLFUNC(GetString)
	GETOPENGLFUNC(GetError)

	GETOPENGLFUNC(DepthFunc)
	GETOPENGLFUNC(DepthMask)

	GETOPENGLFUNC(Viewport)

	GETOPENGLFUNC(DrawArrays)
	GETOPENGLFUNC(DrawElements)

	GETOPENGLFUNC(PixelStorei)
	GETOPENGLFUNC(ReadPixels)

	GETOPENGLFUNC(TexParameteri)
	GETOPENGLFUNC(TexImage2D)
	GETOPENGLFUNC(TexSubImage2D)

	GETOPENGLFUNC(GenTextures)
	GETOPENGLFUNC(DeleteTextures)
	GETOPENGLFUNC(BindTexture)

	GETOPENGLFUNC(CopyTexImage2D)
	GETOPENGLFUNC(CopyTexSubImage2D)

	return true;
}

boolean GLBackend_LoadLegacyFunctions(void)
{
#ifndef HAVE_GLES2
	GETOPENGLFUNC(MatrixMode)
	GETOPENGLFUNC(Viewport)
	GETOPENGLFUNC(PushMatrix)
	GETOPENGLFUNC(PopMatrix)
	GETOPENGLFUNC(LoadIdentity)
	GETOPENGLFUNC(MultMatrixf)
	GETOPENGLFUNC(Rotatef)
	GETOPENGLFUNC(Scalef)
	GETOPENGLFUNC(Translatef)

	GETOPENGLFUNC(ShadeModel)
	GETOPENGLFUNC(Lightfv)
	GETOPENGLFUNC(LightModelfv)
	GETOPENGLFUNC(Materialfv)
#endif

	return true;
}

// ==========================================================================
//                                                                  FUNCTIONS
// ==========================================================================

static void SetBlendEquation(GLenum mode)
{
	gl_BlendEquation(mode);
}

static void SetBlendMode(FBITFIELD flags)
{
	// Set blending function
	switch (flags)
	{
		case PF_Translucent & PF_Blending:
			gl_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
			break;
		case PF_Masked & PF_Blending:
			// Hurdler: does that mean lighting is only made by alpha src?
			// it sounds ok, but not for polygonsmooth
			gl_BlendFunc(GL_SRC_ALPHA, GL_ZERO);                // 0 alpha = holes in texture
			break;
		case PF_Additive & PF_Blending:
		case PF_Subtractive & PF_Blending:
		case PF_ReverseSubtract & PF_Blending:
			gl_BlendFunc(GL_SRC_ALPHA, GL_ONE); // src * alpha + dest
			break;
		case PF_Environment & PF_Blending:
			gl_BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case PF_Multiplicative & PF_Blending:
			gl_BlendFunc(GL_DST_COLOR, GL_ZERO);
			break;
		case PF_Fog & PF_Fog:
			// Sryder: Fog
			// multiplies input colour by input alpha, and destination colour by input colour, then adds them
			gl_BlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
			break;
		default: // must be 0, otherwise it's an error
			// No blending
			gl_BlendFunc(GL_ONE, GL_ZERO);   // the same as no blending
			break;
	}

	// Set blending equation
	switch (flags)
	{
		case PF_Subtractive & PF_Blending:
			SetBlendEquation(GL_FUNC_SUBTRACT);
			break;
		case PF_ReverseSubtract & PF_Blending:
			// good for shadow
			// not really but what else ?
			SetBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			break;
		default:
			SetBlendEquation(GL_FUNC_ADD);
			break;
	}

#ifdef HAVE_GLES2
	alpha_test = true;
	alpha_threshold = 0.5f;
#endif

	// Alpha test
	switch (flags)
	{
		case PF_Masked & PF_Blending:
#ifndef HAVE_GLES2
			gl_AlphaFunc(GL_GREATER, 0.5f);
#endif
			break;
		case PF_Translucent & PF_Blending:
		case PF_Additive & PF_Blending:
		case PF_Subtractive & PF_Blending:
		case PF_ReverseSubtract & PF_Blending:
		case PF_Environment & PF_Blending:
		case PF_Multiplicative & PF_Blending:
#ifdef HAVE_GLES2
			alpha_threshold = 0.0f;
#else
			gl_AlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
			break;
		case PF_Fog & PF_Fog:
#ifdef HAVE_GLES2
			alpha_test = false;
#else
			gl_AlphaFunc(GL_ALWAYS, 0.0f); // Don't discard zero alpha fragments
#endif
			break;
		default:
#ifndef HAVE_GLES2
			gl_AlphaFunc(GL_GREATER, 0.5f);
#endif
			break;
	}
}

// PF_Masked - we could use an ALPHA_TEST of GL_EQUAL, and alpha ref of 0,
//             is it faster when pixels are discarded ?

void GLBackend_SetBlend(FBITFIELD PolyFlags)
{
	FBITFIELD Xor = CurrentPolyFlags^PolyFlags;

	if (Xor & (PF_Blending|PF_RemoveYWrap|PF_ForceWrapX|PF_ForceWrapY|PF_Occlude|PF_NoTexture|PF_Modulated|PF_NoDepthTest|PF_Decal|PF_Invisible))
	{
		if (Xor & PF_Blending) // if blending mode must be changed
			SetBlendMode(PolyFlags & PF_Blending);

#ifndef HAVE_GLES2
		if (Xor & PF_NoAlphaTest)
		{
			if (PolyFlags & PF_NoAlphaTest)
				gl_Disable(GL_ALPHA_TEST);
			else
				gl_Enable(GL_ALPHA_TEST);      // discard 0 alpha pixels (holes in texture)
		}
#endif

		if (Xor & PF_Decal)
		{
			if (PolyFlags & PF_Decal)
				gl_Enable(GL_POLYGON_OFFSET_FILL);
			else
				gl_Disable(GL_POLYGON_OFFSET_FILL);
		}

		if (Xor & PF_NoDepthTest)
		{
			if (PolyFlags & PF_NoDepthTest)
				gl_DepthFunc(GL_ALWAYS);
			else
				gl_DepthFunc(GL_LEQUAL);
		}

		if (Xor & PF_RemoveYWrap)
		{
			if (PolyFlags & PF_RemoveYWrap)
				GPU->SetClamp(GL_TEXTURE_WRAP_T);
		}

		if (Xor & PF_ForceWrapX)
		{
			if (PolyFlags & PF_ForceWrapX)
				gl_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}

		if (Xor & PF_ForceWrapY)
		{
			if (PolyFlags & PF_ForceWrapY)
				gl_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

#ifndef HAVE_GLES2
		if (Xor & PF_Modulated)
		{
			if (PolyFlags & PF_Modulated)
			{   // mix texture colour with Surface->PolyColor
				gl_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
			else
			{   // colour from texture is unchanged before blending
				gl_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
#endif

		if (Xor & PF_Occlude) // depth test but (no) depth write
		{
			if (PolyFlags&PF_Occlude)
			{
				gl_DepthMask(1);
			}
			else
				gl_DepthMask(0);
		}
		////Hurdler: not used if we don't define POLYSKY
		if (Xor & PF_Invisible)
		{
			if (PolyFlags&PF_Invisible)
				gl_BlendFunc(GL_ZERO, GL_ONE);         // transparent blending
			else
			{   // big hack: (TODO: manage that better)
				// we test only for PF_Masked because PF_Invisible is only used
				// (for now) with it (yeah, that's crappy, sorry)
				if ((PolyFlags&PF_Blending)==PF_Masked)
					gl_BlendFunc(GL_SRC_ALPHA, GL_ZERO);
			}
		}
		if (PolyFlags & PF_NoTexture)
			GPU->SetNoTexture();
	}

	CurrentPolyFlags = PolyFlags;
}

INT32 GLBackend_GetAlphaTestShader(INT32 type)
{
#ifdef HAVE_GLES2
	switch (type)
	{
		case SHADER_DEFAULT: return SHADER_ALPHA_TEST;
		case SHADER_FLOOR: return SHADER_FLOOR_ALPHA_TEST;
		case SHADER_WALL: return SHADER_WALL_ALPHA_TEST;
		case SHADER_SPRITE: return SHADER_SPRITE_ALPHA_TEST;
		case SHADER_MODEL: return SHADER_MODEL_ALPHA_TEST;
		case SHADER_MODEL_LIGHTING: return SHADER_MODEL_LIGHTING_ALPHA_TEST;
		case SHADER_WATER: return SHADER_WATER_ALPHA_TEST;
		default: break;
	}
#endif

	return type;
}

INT32 GLBackend_InvertAlphaTestShader(INT32 type)
{
#ifdef HAVE_GLES2
	switch (type)
	{
		case SHADER_ALPHA_TEST: return SHADER_DEFAULT;
		case SHADER_FLOOR_ALPHA_TEST: return SHADER_FLOOR;
		case SHADER_WALL_ALPHA_TEST: return SHADER_WALL;
		case SHADER_SPRITE_ALPHA_TEST: return SHADER_SPRITE;
		case SHADER_MODEL_ALPHA_TEST: return SHADER_MODEL;
		case SHADER_MODEL_LIGHTING_ALPHA_TEST: return SHADER_MODEL_LIGHTING;
		case SHADER_WATER_ALPHA_TEST: return SHADER_WATER;
		default: break;
	}
#endif

	return type;
}

INT32 GLBackend_GetShaderType(INT32 type)
{
#ifdef GL_SHADERS
	// If using model lighting, set the appropriate shader.
	// However don't override a custom shader.
	if (type == SHADER_MODEL && model_lighting
	&& !(gl_shaders[SHADER_MODEL].custom && !gl_shaders[SHADER_MODEL_LIGHTING].custom))
		type = SHADER_MODEL_LIGHTING;
#endif

#ifdef HAVE_GLES2
	if (!alpha_test)
		return type;

	switch (type)
	{
		case SHADER_DEFAULT:
		case SHADER_FLOOR:
		case SHADER_WALL:
		case SHADER_SPRITE:
		case SHADER_MODEL:
		case SHADER_MODEL_LIGHTING:
		case SHADER_WATER:
		{
			INT32 newshader = GLBackend_GetAlphaTestShader(type);
			if (!(gl_shaders[type].custom && !gl_shaders[newshader].custom))
				return newshader;
		}
		default:
			break;
	}
#endif

	return type;
}

void GLBackend_SetSurface(INT32 w, INT32 h)
{
	GPU->SetModelView(w, h);
	GPU->SetStates();

	gl_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static boolean version_checked = false;

boolean GLBackend_InitContext(void)
{
	if (!GLBackend_LoadCommonFunctions())
		return false;

	if (!version_checked)
	{
		gl_version = gl_GetString(GL_VERSION);
		gl_renderer = gl_GetString(GL_RENDERER);

		GL_DBG_Printf("OpenGL version: %s\n", gl_version);
		GL_DBG_Printf("GPU: %s\n", gl_renderer);

#if !defined(__ANDROID__)
		if (strcmp((const char*)gl_renderer, "GDI Generic") == 0 &&
			strcmp((const char*)gl_version, "1.1.0") == 0)
		{
			// Oh no... Windows gave us the GDI Generic rasterizer, so something is wrong...
			// The game will crash later on when unsupported OpenGL commands are encountered.
			// Instead of a nondescript crash, show a more informative error message.
			// Also set the renderer variable back to software so the next launch won't
			// repeat this error.
			CV_StealthSet(&cv_renderer, "Software");
			I_Error("OpenGL Error: Failed to access the GPU. Possible reasons include:\n"
					"- GPU vendor has dropped OpenGL support on your GPU and OS. (Old GPU?)\n"
					"- GPU drivers are missing or broken. You may need to update your drivers.");
		}
#endif

		version_checked = true;
	}

	if (gl_extensions == NULL)
		GLExtension_Init();

	return true;
}

void GLBackend_DeleteModelData(void)
{
	while (ModelListHead)
	{
		GLModelList *pModel = ModelListHead;

		if (pModel->model && pModel->model->meshes)
			GLModel_DeleteVBOs(pModel->model);

		ModelListHead = pModel->next;
		free(pModel);
	}

	ModelListTail = ModelListHead = NULL;
}

void GLBackend_RecreateContext(void)
{
	while (TexCacheHead)
	{
		FTextureInfo *pTexInfo = TexCacheHead;
		GLMipmap_t *texture = pTexInfo->texture;

		if (pTexInfo->downloaded)
		{
			gl_DeleteTextures(1, (GLuint *)&pTexInfo->downloaded);
			pTexInfo->downloaded = 0;
		}

		if (texture)
			texture->downloaded = 0;

		TexCacheHead = pTexInfo->next;
		free(pTexInfo);
	}

	TexCacheTail = TexCacheHead = NULL;

	GLTexture_FlushScreen();
	gl_DeleteTextures(1, &blank_texture_num);
	blank_texture_num = 0;
	tex_downloaded = 0;

#ifdef HAVE_GL_FRAMEBUFFER
	if (GLExtension_framebuffer_object)
	{
		// Unbind the framebuffer and renderbuffer
		gl_BindFramebuffer(GL_FRAMEBUFFER, 0);
		gl_BindRenderbuffer(GL_RENDERBUFFER, 0);

		FramebufferObject = FramebufferTexture = 0;
		RenderbufferObject = 0;
	}
#endif

	while (ModelListHead)
	{
		GLModelList *pModel = ModelListHead;

		if (pModel->model && pModel->model->meshes)
			GLModel_ClearVBOs(pModel->model);

		ModelListHead = pModel->next;
		free(pModel);
	}

	ModelListTail = ModelListHead = NULL;

#ifdef GL_SHADERS
	Shader_CleanPrograms();
	Shader_Compile();
#endif
}

void GLBackend_SetPalette(RGBA_t *palette)
{
	size_t palsize = sizeof(RGBA_t) * 256;

	// on a palette change, you have to reload all of the textures
	if (memcmp(&myPaletteData, palette, palsize))
	{
		memcpy(&myPaletteData, palette, palsize);
		GLTexture_Flush();
	}
}

static size_t lerpBufferSize = 0;
float *vertBuffer = NULL;
float *normBuffer = NULL;

static size_t lerpTinyBufferSize = 0;
short *vertTinyBuffer = NULL;
char *normTinyBuffer = NULL;

// Static temporary buffer for doing frame interpolation
// 'size' is the vertex size
void GLModel_AllocLerpBuffer(size_t size)
{
	if (lerpBufferSize >= size)
		return;

	if (vertBuffer != NULL)
		free(vertBuffer);

	if (normBuffer != NULL)
		free(normBuffer);

	lerpBufferSize = size;
	vertBuffer = malloc(lerpBufferSize);
	normBuffer = malloc(lerpBufferSize);
}

// Static temporary buffer for doing frame interpolation
// 'size' is the vertex size
void GLModel_AllocLerpTinyBuffer(size_t size)
{
	if (lerpTinyBufferSize >= size)
		return;

	if (vertTinyBuffer != NULL)
		free(vertTinyBuffer);

	if (normTinyBuffer != NULL)
		free(normTinyBuffer);

	lerpTinyBufferSize = size;
	vertTinyBuffer = malloc(lerpTinyBufferSize);
	normTinyBuffer = malloc(lerpTinyBufferSize / 2);
}

static void CreateModelVBO(mesh_t *mesh, mdlframe_t *frame)
{
	int bufferSize = sizeof(vbo64_t)*mesh->numTriangles * 3;
	vbo64_t *buffer = (vbo64_t*)malloc(bufferSize);
	vbo64_t *bufPtr = buffer;

	float *vertPtr = frame->vertices;
	float *normPtr = frame->normals;
	float *tanPtr = frame->tangents;
	float *uvPtr = mesh->uvs;
	float *lightPtr = mesh->lightuvs;
	char *colorPtr = frame->colors;

	int i;
	for (i = 0; i < mesh->numTriangles * 3; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = *normPtr++;
		bufPtr->ny = *normPtr++;
		bufPtr->nz = *normPtr++;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr != NULL)
		{
			bufPtr->tan0 = *tanPtr++;
			bufPtr->tan1 = *tanPtr++;
			bufPtr->tan2 = *tanPtr++;
		}

		if (lightPtr != NULL)
		{
			bufPtr->s1 = *lightPtr++;
			bufPtr->t1 = *lightPtr++;
		}

		if (colorPtr)
		{
			bufPtr->r = *colorPtr++;
			bufPtr->g = *colorPtr++;
			bufPtr->b = *colorPtr++;
			bufPtr->a = *colorPtr++;
		}
		else
		{
			bufPtr->r = 255;
			bufPtr->g = 255;
			bufPtr->b = 255;
			bufPtr->a = 255;
		}

		bufPtr++;
	}

	gl_GenBuffers(1, &frame->vboID);
	gl_BindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	gl_BufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	gl_BindBuffer(GL_ARRAY_BUFFER, 0);
}

static void CreateModelVBOTiny(mesh_t *mesh, tinyframe_t *frame)
{
	int bufferSize = sizeof(vbotiny_t)*mesh->numTriangles * 3;
	vbotiny_t *buffer = (vbotiny_t*)malloc(bufferSize);
	vbotiny_t *bufPtr = buffer;

	short *vertPtr = frame->vertices;
	char *normPtr = frame->normals;
	float *uvPtr = mesh->uvs;
	char *tanPtr = frame->tangents;

	int i;
	for (i = 0; i < mesh->numVertices; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = *normPtr++;
		bufPtr->ny = *normPtr++;
		bufPtr->nz = *normPtr++;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr)
		{
			bufPtr->tanx = *tanPtr++;
			bufPtr->tany = *tanPtr++;
			bufPtr->tanz = *tanPtr++;
		}

		bufPtr++;
	}

	gl_GenBuffers(1, &frame->vboID);
	gl_BindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	pglBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	gl_BindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLModel_GenerateVBOs(model_t *model)
{
	int i, j;

	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				if (frame->vboID)
					gl_DeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBO(mesh, frame);
			}
		}
		else if (mesh->tinyframes)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				if (frame->vboID)
					gl_DeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBOTiny(mesh, frame);
			}
		}
	}

	if (ModelListTail)
	{
		ModelListTail->next = calloc(1, sizeof(GLModelList));
		ModelListTail = ModelListTail->next;
	}
	else
		ModelListTail = ModelListHead = calloc(1, sizeof(GLModelList));

	ModelListTail->model = model;
	ModelListTail->next = NULL;
}

void GLModel_ClearVBOs(model_t *model)
{
	int i, j;

	if (!model->hasVBOs)
		return;

	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				frame->vboID = 0;
			}
		}
		else if (mesh->tinyframes)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				frame->vboID = 0;
			}
		}
	}

	model->hasVBOs = false;
}

void GLModel_DeleteVBOs(model_t *model)
{
	int i, j;

	if (!model->hasVBOs)
		return;

	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				if (frame->vboID)
					gl_DeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
			}
		}
		else if (mesh->tinyframes)
		{
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				if (frame->vboID)
					gl_DeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
			}
		}
	}

	model->hasVBOs = false;
}

void GLTexture_AllocBuffer(GLMipmap_t *pTexInfo)
{
	size_t size = pTexInfo->width * pTexInfo->height;
	if (size > TextureBufferSize)
	{
		TextureBuffer = realloc(TextureBuffer, size * sizeof(RGBA_t));
		if (TextureBuffer == NULL)
			I_Error("GLTexture_AllocBuffer: out of memory allocating %s bytes", sizeu1(size * sizeof(RGBA_t)));
		TextureBufferSize = size;
	}
}

void GLTexture_Disable(void)
{
	if (tex_downloaded == blank_texture_num)
		return;

	if (blank_texture_num == 0)
	{
		// Generate a 1x1 white pixel as the blank texture
		UINT8 whitepixel[4] = {255, 255, 255, 255};
		gl_GenTextures(1, &blank_texture_num);
		gl_BindTexture(GL_TEXTURE_2D, blank_texture_num);
		gl_TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitepixel);
	}
	else
		gl_BindTexture(GL_TEXTURE_2D, blank_texture_num);

	tex_downloaded = blank_texture_num;
}

// -----------------+
// Flush            : Flush OpenGL textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void GLTexture_Flush(void)
{
	while (TexCacheHead)
	{
		FTextureInfo *pTexInfo = TexCacheHead;
		GLMipmap_t *texture = pTexInfo->texture;

		if (pTexInfo->downloaded)
		{
			gl_DeleteTextures(1, (GLuint *)&pTexInfo->downloaded);
			pTexInfo->downloaded = 0;
		}

		if (texture)
			texture->downloaded = 0;

		TexCacheHead = pTexInfo->next;
		free(pTexInfo);
	}

	TexCacheTail = TexCacheHead = NULL; //Hurdler: well, TexCacheHead is already NULL
	tex_downloaded = 0;

	free(TextureBuffer);
	TextureBuffer = NULL;
	TextureBufferSize = 0;
}


// Sryder:	This needs to be called whenever the screen changes resolution in order to reset the screen textures to use
//			a new size
void GLTexture_FlushScreen(void)
{
	if (screentexture)
		gl_DeleteTextures(1, &screentexture);
	if (startScreenWipe)
		gl_DeleteTextures(1, &startScreenWipe);
	if (endScreenWipe)
		gl_DeleteTextures(1, &endScreenWipe);
	if (finalScreenTexture)
		gl_DeleteTextures(1, &finalScreenTexture);

	screentexture = 0;
	startScreenWipe = 0;
	endScreenWipe = 0;
	finalScreenTexture = 0;
}


// -----------------+
// SetFilterMode    : Sets texture filtering mode
//                  :
// -----------------+
void GLTexture_SetFilterMode(INT32 mode)
{
	MipmapEnabled = GL_FALSE;

	switch (mode)
	{
		case HWD_SET_TEXTUREFILTER_TRILINEAR:
			mag_filter = GL_LINEAR;
			if (MipmapSupported)
			{
				min_filter = GL_LINEAR_MIPMAP_LINEAR;
				MipmapEnabled = GL_TRUE;
			}
			else
				min_filter = GL_LINEAR;
			break;
		case HWD_SET_TEXTUREFILTER_BILINEAR:
			min_filter = mag_filter = GL_LINEAR;
			break;
		case HWD_SET_TEXTUREFILTER_POINTSAMPLED:
			min_filter = mag_filter = GL_NEAREST;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED1:
			min_filter = GL_NEAREST;
			mag_filter = GL_LINEAR;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED2:
			min_filter = GL_LINEAR;
			mag_filter = GL_NEAREST;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED3:
			mag_filter = GL_NEAREST;
			if (MipmapSupported)
			{
				min_filter = GL_LINEAR_MIPMAP_LINEAR;
				MipmapEnabled = GL_TRUE;
			}
			else
				min_filter = GL_LINEAR;
			break;
		default:
			mag_filter = GL_LINEAR;
			min_filter = GL_NEAREST;
	}
}

// -----------------------+
// GetMemoryUsage         : calculates total memory usage by textures
// Returns                : total memory amount used by textures, excluding mipmaps
// -----------------------+
INT32 GLTexture_GetMemoryUsage(FTextureInfo *head)
{
	INT32 res = 0;

	while (head)
	{
		// Figure out the correct bytes-per-pixel for this texture
		// This follows format2bpp in hw_cache.c
		int bpp = 1;
		int format = head->format;
		if (format == GL_TEXFMT_RGBA)
			bpp = 4;
		else if (format == GL_TEXFMT_ALPHA_INTENSITY_88 || format == GL_TEXFMT_AP_88)
			bpp = 2;

		// Add it up!
		res += head->height*head->width*bpp;
		head = head->next;
	}

	return res;
}

#ifdef HAVE_GL_FRAMEBUFFER
void GLFramebuffer_Generate(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	// Generate the framebuffer
	if (FramebufferObject == 0)
		gl_GenFramebuffers(1, &FramebufferObject);

	if (gl_CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
		GLFramebuffer_GenerateAttachments();
}

void GLFramebuffer_Delete(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	// Unbind the framebuffer and renderbuffer
	gl_BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl_BindRenderbuffer(GL_RENDERBUFFER, 0);

	if (FramebufferObject)
		gl_DeleteFramebuffers(1, &FramebufferObject);

	GLFramebuffer_DeleteAttachments();
	FramebufferObject = 0;
}

static boolean CheckRenderbuffer(void)
{
	GLenum error = pglGetError();
	INT32 i = ((INT32)RenderbufferDepthBits) + 1;
	boolean test = (i < NumRenderbufferFormats);

	if (error == GL_NO_ERROR)
		return true;

	GL_DBG_Printf("glRenderbufferStorage: %s\n", GetGLError(error));

	if (test)
	{
		while (error != GL_NO_ERROR)
		{
			gl_RenderbufferStorage(GL_RENDERBUFFER, RenderbufferFormats[i++], screen_width, screen_height);
			error = pglGetError();

			if (i == NumRenderbufferFormats)
			{
				if (error != GL_NO_ERROR)
					test = false;
				break;
			}
		}
	}

	if (!test)
	{
		i = (NumRenderbufferFormats - 1);

		while (error != GL_NO_ERROR)
		{
			gl_RenderbufferStorage(GL_RENDERBUFFER, RenderbufferFormats[i--], screen_width, screen_height);
			error = pglGetError();

			if (i < 0)
				return false;
		}
	}

	return true;
}

void GLFramebuffer_GenerateAttachments(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	// Bind the framebuffer
	gl_BindFramebuffer(GL_FRAMEBUFFER, FramebufferObject);

	// Generate the framebuffer texture
	if (FramebufferTexture == 0)
	{
		gl_GenTextures(1, &FramebufferTexture);
		gl_BindTexture(GL_TEXTURE_2D, FramebufferTexture);
		gl_TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen_width, screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		gl_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl_TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl_BindTexture(GL_TEXTURE_2D, 0);

		// Attach the framebuffer texture to the framebuffer
		gl_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FramebufferTexture, 0);
	}

	// Generate the renderbuffer
	if (RenderbufferObject == 0)
	{
		gl_GenRenderbuffers(1, &RenderbufferObject);

		gl_BindRenderbuffer(GL_RENDERBUFFER, RenderbufferObject);
		gl_RenderbufferStorage(GL_RENDERBUFFER, RenderbufferFormats[RenderbufferDepthBits], screen_width, screen_height);

		if (CheckRenderbuffer())
		{
			// Attach the renderbuffer to the framebuffer
			gl_FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RenderbufferObject);

			// Clear the renderbuffer
			GPU->ClearBuffer(true, true, NULL);
		}
		else
			RenderToFramebuffer = GL_FALSE;

		gl_BindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	// Unbind the framebuffer
	gl_BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer_DeleteAttachments(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	// Unbind the framebuffer and renderbuffer
	gl_BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl_BindRenderbuffer(GL_RENDERBUFFER, 0);

	if (FramebufferTexture)
		gl_DeleteTextures(1, &FramebufferTexture);

	if (RenderbufferObject)
		gl_DeleteRenderbuffers(1, &RenderbufferObject);

	FramebufferTexture = 0;
	RenderbufferObject = 0;
}

void GLFramebuffer_Enable(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	if (RenderbufferDepthBits != LastRenderbufferDepthBits)
	{
		if (RenderbufferObject)
			gl_DeleteRenderbuffers(1, &RenderbufferObject);

		RenderbufferObject = 0;
		LastRenderbufferDepthBits = RenderbufferDepthBits;
	}

	if (FramebufferObject == 0)
		GLFramebuffer_Generate();
	else if (FramebufferTexture == 0 || RenderbufferObject == 0)
		GLFramebuffer_GenerateAttachments();

	if (RenderToFramebuffer == GL_FALSE)
		return;

	gl_BindFramebuffer(GL_FRAMEBUFFER, FramebufferObject);
	gl_BindRenderbuffer(GL_RENDERBUFFER, RenderbufferObject);
}

void GLFramebuffer_Disable(void)
{
	if (!GLExtension_framebuffer_object)
		return;

	gl_BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl_BindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GLFramebuffer_SetDepth(INT32 depth)
{
	RenderbufferDepthBits = min(max(depth, 0), NumRenderbufferFormats-1);
}
#endif

void GLBackend_ReadRect(INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT16 *dst_data)
{
	INT32 i;

	if (dst_stride == width*3)
	{
		GLubyte *top = (GLvoid*)dst_data, *bottom = top + dst_stride * (height - 1);
		GLubyte *row = malloc(dst_stride);

		if (!row)
			return;

		gl_PixelStorei(GL_PACK_ALIGNMENT, 1);
		gl_ReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst_data);
		gl_PixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (i = 0; i < height/2; i++)
		{
			memcpy(row, top, dst_stride);
			memcpy(top, bottom, dst_stride);
			memcpy(bottom, row, dst_stride);
			top += dst_stride;
			bottom -= dst_stride;
		}

		free(row);
	}
	else
	{
		GLubyte *image = malloc(width*height*3*sizeof(*image));
		if (!image)
			return;

		gl_PixelStorei(GL_PACK_ALIGNMENT, 1);
		gl_ReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
		gl_PixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (i = height-1; i >= 0; i--)
		{
			INT32 j;
			for (j = 0; j < width; j++)
			{
				dst_data[(height-1-i)*width+j] =
				(UINT16)(
				                 ((image[(i*width+j)*3]>>3)<<11) |
				                 ((image[(i*width+j)*3+1]>>2)<<5) |
				                 ((image[(i*width+j)*3+2]>>3)));
			}
		}

		free(image);
	}
}

void GLBackend_ReadRectRGBA(INT32 x, INT32 y, INT32 width, INT32 height, UINT32 *dst_data)
{
	INT32 i, row_width = (width * sizeof(UINT32));
	UINT32 *src_data = malloc(row_width * height);

	if (!src_data)
		return;

	gl_PixelStorei(GL_PACK_ALIGNMENT, 1);
	gl_ReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *)src_data);
	gl_PixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (i = height-1; i >= 0; i--)
		memcpy(&dst_data[i * width], &src_data[(height-i-1) * width], row_width);

	free(src_data);
}

void GLExtension_Init(void)
{
	INT32 i = 0;

	gl_extensions = gl_GetString(GL_EXTENSIONS);

	GL_DBG_Printf("Extensions: ");
	PrintExtensions(gl_extensions);

#ifdef HAVE_GLES2
	GLExtension_vertex_buffer_object = true;
	GLExtension_vertex_program = true;
	GLExtension_fragment_program = true;
#endif

	while (ExtensionList[i].name)
	{
		const FExtensionList *ext = &ExtensionList[i++];
		boolean *extension = ext->extension;

		if (*extension)
			continue;

		if (GLExtension_Available(ext->name))
		{
			(*extension) = true;
			GL_DBG_Printf("Extension %s is supported\n", ext->name);
		}
		else
		{
			(*extension) = false;
			GL_DBG_Printf("Extension %s is unsupported\n", ext->name);
		}
	}

#ifdef GL_SHADERS
	if (GLExtension_vertex_program && GLExtension_fragment_program && GLBackend_GetFunction("glUseProgram"))
		GLExtension_shaders = true;
#endif

	if (GLExtension_texture_filter_anisotropic)
	{
		gl_GetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);

		if (!maximumAnisotropy)
		{
			GLExtension_texture_filter_anisotropic = false;
			maximumAnisotropy = 1;
		}
	}
	else
		maximumAnisotropy = 1;

	glanisotropicmode_cons_t[1].value = maximumAnisotropy;
}

boolean GLExtension_Available(const char *extension)
{
#ifdef HAVE_SDL
	return SDL_GL_ExtensionSupported(extension) == SDL_TRUE ? true : false;
#else
	const GLubyte *start = gl_extensions;
	GLubyte       *where, *terminator;
	if (!extension || !start)
		return false;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return false;
	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;
		start = terminator;
	}
	return false;
#endif
}

static boolean CheckFunctionList(const char **list)
{
	char *funcname = NULL;

	INT32 i = 0;

	while (list[i])
	{
		size_t len = strlen(list[i]) + 3;

		funcname = realloc(funcname, len);
		snprintf(funcname, len, "gl%s", list[i++]);

		if (!GLBackend_GetFunction(funcname))
		{
			free(funcname);
			return false;
		}
	}

	if (funcname)
		free(funcname);

	return true;
}

#define EXTUNSUPPORTED(ext) { \
	GL_DBG_Printf("%s is unsupported\n", #ext); \
	ext = false; }

boolean GLExtension_LoadFunctions(void)
{
	if (GLExtension_multitexture)
	{
		const char *list[] =
		{
			"ActiveTexture",
#ifndef HAVE_GLES2
			"ClientActiveTexture",
#endif
			NULL
		};

		if (CheckFunctionList(list))
		{
			GETOPENGLFUNC(ActiveTexture)
#ifndef HAVE_GLES2
			GETOPENGLFUNC(ClientActiveTexture)
#endif
		}
		else
			EXTUNSUPPORTED(GLExtension_multitexture);
	}

	if (GLExtension_vertex_buffer_object)
	{
		const char *list[] =
		{
			"GenBuffers",
			"BindBuffer",
			"BufferData",
			"DeleteBuffers",
			NULL
		};

		if (CheckFunctionList(list))
		{
			GETOPENGLFUNC(GenBuffers)
			GETOPENGLFUNC(BindBuffer)
			GETOPENGLFUNC(BufferData)
			GETOPENGLFUNC(DeleteBuffers)
		}
		else
			EXTUNSUPPORTED(GLExtension_vertex_buffer_object);
	}

#ifdef HAVE_GL_FRAMEBUFFER
	if (GLExtension_framebuffer_object)
	{
		const char *list[] =
		{
			"GenFramebuffers",
			"BindFramebuffer",
			"DeleteFramebuffers",
			"FramebufferTexture2D",
			"CheckFramebufferStatus",
			"GenRenderbuffers",
			"BindRenderbuffer",
			"DeleteRenderbuffers",
			"RenderbufferStorage",
			"FramebufferRenderbuffer",
			NULL
		};

		if (CheckFunctionList(list))
		{
			GETOPENGLFUNC(GenFramebuffers);
			GETOPENGLFUNC(BindFramebuffer);
			GETOPENGLFUNC(DeleteFramebuffers);
			GETOPENGLFUNC(FramebufferTexture2D);
			GETOPENGLFUNC(CheckFramebufferStatus);
			GETOPENGLFUNC(GenRenderbuffers);
			GETOPENGLFUNC(BindRenderbuffer);
			GETOPENGLFUNC(DeleteRenderbuffers);
			GETOPENGLFUNC(RenderbufferStorage);
			GETOPENGLFUNC(FramebufferRenderbuffer);
		}
		else
			EXTUNSUPPORTED(GLExtension_framebuffer_object);
	}
#endif

	return true;
}

#undef EXTUNSUPPORTED

static void PrintExtensions(const GLubyte *extensions)
{
	size_t size = strlen((const char *)extensions) + 1;
	char *tk, *ext = calloc(size, sizeof(char));

	memcpy(ext, extensions, size);
	tk = strtok(ext, " ");

	while (tk)
	{
		GL_DBG_Printf("%s", tk);
		tk = strtok(NULL, " ");
		if (tk)
			GL_DBG_Printf(" ", tk);
	}

	GL_DBG_Printf("\n");
	free(ext);
}


// -----------------+
// GL_DBG_Printf    : Output debug messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+

#ifdef DEBUG_TO_FILE
FILE *gllogstream;
#endif

#define DEBUG_TO_CONSOLE

void GL_DBG_Printf(const char *format, ...)
{
#if defined(DEBUG_TO_CONSOLE) || defined(DEBUG_TO_FILE)
	char str[4096] = "";
	va_list arglist;

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

#ifdef DEBUG_TO_CONSOLE
	I_OutputMsg("%s", str);
#endif

#ifdef DEBUG_TO_FILE
	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");
	fwrite(str, strlen(str), 1, gllogstream);
#endif
#else // defined(DEBUG_TO_CONSOLE) || defined(DEBUG_TO_FILE)
	(void)format;
#endif
}

// -----------------+
// GL_MSG_Warning   : Raises a warning.
//                  :
// Returns          :
// -----------------+

void GL_MSG_Warning(const char *format, ...)
{
	char str[4096] = "";
	va_list arglist;

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

	CONS_Alert(CONS_WARNING, "%s", str);

#ifdef DEBUG_TO_FILE
	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");
	fwrite(str, strlen(str), 1, gllogstream);
#endif
}

// -----------------+
// GL_MSG_Error     : Raises an error.
//                  :
// Returns          :
// -----------------+

char *lastglerror = NULL;

void GL_MSG_Error(const char *format, ...)
{
	char str[4096] = "";
	va_list arglist;

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

	CONS_Alert(CONS_ERROR, "%s", str);

	if (lastglerror)
		free(lastglerror);
	lastglerror = strcpy(malloc(strlen(str) + 1), str);

#ifdef DEBUG_TO_FILE
	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");
	fwrite(str, strlen(str), 1, gllogstream);
#endif
}
