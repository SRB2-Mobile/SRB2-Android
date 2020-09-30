// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2020 by Sonic Team Junior.
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

#include <stdarg.h>

const GLubyte *gl_version = NULL;
const GLubyte *gl_renderer = NULL;
const GLubyte *gl_extensions = NULL;

// ==========================================================================
//                                                                  CONSTANTS
// ==========================================================================

GLuint NOTEXTURE_NUM = 0;
float NEAR_CLIPPING_PLANE = NZCLIP_PLANE;

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

RGBA_t  myPaletteData[256];
GLint   screen_width    = 0;
GLint   screen_height   = 0;
GLbyte  screen_depth    = 0;
GLint   textureformatGL = 0;
GLint maximumAnisotropy = 0;

GLboolean MipMap = GL_FALSE;
GLint min_filter = GL_LINEAR;
GLint mag_filter = GL_LINEAR;
GLint anisotropic_filter = 0;

FTextureInfo *gl_cachetail = NULL;
FTextureInfo *gl_cachehead = NULL;

GLuint      tex_downloaded  = 0;
GLfloat     fov             = 90.0f;
FBITFIELD   CurrentPolyFlags;

// Sryder:	NextTexAvail is broken for these because palette changes or changes to the texture filter or antialiasing
//			flush all of the stored textures, leaving them unavailable at times such as between levels
//			These need to start at 0 and be set to their number, and be reset to 0 when deleted so that intel GPUs
//			can know when the textures aren't there, as textures are always considered resident in their virtual memory
GLuint screentexture = 0;
GLuint startScreenWipe = 0;
GLuint endScreenWipe = 0;
GLuint finalScreenTexture = 0;

// ==========================================================================
//                                                           OPENGL FUNCTIONS
// ==========================================================================

#ifndef STATIC_OPENGL
/* Miscellaneous */
PFNglClear pglClear;
PFNglGetFloatv pglGetFloatv;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;
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

/* 1.3 functions for multitexturing */
PFNglActiveTexture pglActiveTexture;
#endif

//
// Multi-texturing
//

#ifndef HAVE_GLES2
#ifndef STATIC_OPENGL
PFNglClientActiveTexture pglClientActiveTexture;
#endif
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
PFNglMateriali pglMateriali;

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

boolean GLBackend_LoadCommonFunctions(void)
{
#define GETOPENGLFUNC(func) \
	p ## gl ## func = GLBackend_GetFunction("gl" #func); \
	if (!(p ## gl ## func)) \
	{ \
		GL_MSG_Error("failed to get OpenGL function: %s", #func); \
		return false; \
	} \

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
	GETOPENGLFUNC(Materiali)
#endif

	return true;
}

#undef GETOPENGLFUNC

// ==========================================================================
//                                                                  FUNCTIONS
// ==========================================================================

void SetSurface(INT32 w, INT32 h)
{
	if (gl_version == NULL || gl_renderer == NULL)
	{
		gl_version = pglGetString(GL_VERSION);
		gl_renderer = pglGetString(GL_RENDERER);

		GL_DBG_Printf("OpenGL %s\n", gl_version);
		GL_DBG_Printf("GPU: %s\n", gl_renderer);
	}

	if (gl_extensions == NULL)
	{
		gl_extensions = pglGetString(GL_EXTENSIONS);
		GL_DBG_Printf("Extensions: %s\n", gl_extensions);
	}

	if (GL_ExtensionAvailable("GL_EXT_texture_filter_anisotropic", gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 1;

	SetModelView(w, h);
	SetStates();

	pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* 1.5 functions for buffers */
PFNglGenBuffers pglGenBuffers;
PFNglBindBuffer pglBindBuffer;
PFNglBufferData pglBufferData;
PFNglDeleteBuffers pglDeleteBuffers;

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

	pglGenBuffers(1, &frame->vboID);
	pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	pglBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	pglBindBuffer(GL_ARRAY_BUFFER, 0);
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

	pglGenBuffers(1, &frame->vboID);
	pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	pglBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	pglBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLModel_GenerateVBOs(model_t *model)
{
	int i;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				if (frame->vboID)
					pglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBO(mesh, frame);
			}
		}
		else if (mesh->tinyframes)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				if (frame->vboID)
					pglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBOTiny(mesh, frame);
			}
		}
	}
}

// -----------------+
// Flush            : Flush OpenGL textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void GLTexture_Flush(void)
{
	//GL_DBG_Printf ("GLTexture_Flush()\n");

	while (gl_cachehead)
	{
		if (gl_cachehead->downloaded)
			pglDeleteTextures(1, (GLuint *)&gl_cachehead->downloaded);
		gl_cachehead->downloaded = 0;
		gl_cachehead = gl_cachehead->nextmipmap;
	}
	gl_cachetail = gl_cachehead = NULL; //Hurdler: well, gl_cachehead is already NULL

	tex_downloaded = 0;
}

// -----------------+
// SetFilterMode    : Sets texture filtering mode
//                  :
// -----------------+
void GLTexture_SetFilterMode(INT32 mode)
{
	switch (mode)
	{
		case HWD_SET_TEXTUREFILTER_TRILINEAR:
			min_filter = GL_LINEAR_MIPMAP_LINEAR;
			mag_filter = GL_LINEAR;
			MipMap = GL_TRUE;
			break;
		case HWD_SET_TEXTUREFILTER_BILINEAR:
			min_filter = mag_filter = GL_LINEAR;
			MipMap = GL_FALSE;
			break;
		case HWD_SET_TEXTUREFILTER_POINTSAMPLED:
			min_filter = mag_filter = GL_NEAREST;
			MipMap = GL_FALSE;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED1:
			min_filter = GL_NEAREST;
			mag_filter = GL_LINEAR;
			MipMap = GL_FALSE;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED2:
			min_filter = GL_LINEAR;
			mag_filter = GL_NEAREST;
			MipMap = GL_FALSE;
			break;
		case HWD_SET_TEXTUREFILTER_MIXED3:
			min_filter = GL_LINEAR_MIPMAP_LINEAR;
			mag_filter = GL_NEAREST;
			MipMap = GL_TRUE;
			break;
		default:
			mag_filter = GL_LINEAR;
			min_filter = GL_NEAREST;
	}
}

// -----------------------+
// Texture_GetMemoryUsage : calculates total memory usage by textures
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
		head = head->nextmipmap;
	}

	return res;
}

// -------------------+
// ExtensionAvailable : Look if an OpenGL extension is available
// Returns            : true if extension available
// -------------------+
boolean GL_ExtensionAvailable(const char *extension, const GLubyte *start)
{
#if defined(HAVE_GLES) && defined(HAVE_SDL)
	(void)start;
	return (SDL_GL_ExtensionSupported(extension) == SDL_TRUE ? true : false);
#else
	GLubyte         *where, *terminator;

	if (!extension || !start) return 0;
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

// -----------------+
// GL_DBG_Printf    : Output debug messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+

#ifdef DEBUG_TO_FILE
FILE *gllogstream;
#endif

void GL_DBG_Printf(const char *format, ...)
{
#ifdef DEBUG_TO_FILE
	char str[4096] = "";
	va_list arglist;

	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

	fwrite(str, strlen(str), 1, gllogstream);
#else
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

#ifdef HAVE_SDL
	CONS_Alert(CONS_WARNING, "%s\n", str);
#endif

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

#ifdef HAVE_SDL
	CONS_Alert(CONS_ERROR, "%s\n", str);

	if (lastglerror)
		free(lastglerror);
	lastglerror = strcpy(malloc(strlen(str) + 1), str);
#endif

#ifdef DEBUG_TO_FILE
	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");
	fwrite(str, strlen(str), 1, gllogstream);
#endif
}
