// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1998-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_gles2.c
/// \brief OpenGL ES 2.0 API for Sonic Robo Blast 2

#include <stdarg.h>
#include <math.h>

#include "r_gles.h"
#include "../r_opengl/r_vbo.h"

#include "../shaders/gl_shaders.h"

#include "lzml.h"

#if defined (HWRENDER) && !defined (NOROPENGL)

static GLRGBAFloat white = {1.0f, 1.0f, 1.0f, 1.0f};
static GLRGBAFloat black = {0.0f, 0.0f, 0.0f, 1.0f};

// ==========================================================================
//                                                                  CONSTANTS
// ==========================================================================

#define Deg2Rad(x) ((x) * ((float)M_PIl / 180.0f))

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

fmatrix4_t projMatrix;
fmatrix4_t viewMatrix;
fmatrix4_t modelMatrix;

static GLint viewport[4];

typedef void (R_GL_APIENTRY * PFNglVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
static PFNglVertexAttribPointer pglVertexAttribPointer;

static void VertexAttribPointerInternal(int attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer, const char *function, const int line)
{
	int loc = Shader_AttribLoc(attrib);
	if (loc == -1)
		GL_MSG_Error("VertexAttribPointer: generic vertex attribute %s not bound to any attribute variable (from %s:%d)\n", Shader_AttribLocName(attrib), function, line);
	pglVertexAttribPointer(loc, size, type, normalized, stride, pointer);
}

#define VertexAttribPointer(attrib, size, type, normalized, stride, pointer) VertexAttribPointerInternal(attrib, size, type, normalized, stride, pointer, __FUNCTION__, __LINE__)

boolean GLBackend_LoadFunctions(void)
{
	GLExtension_shaders = true;
	GLExtension_multitexture = true;
	GLExtension_vertex_buffer_object = true;
	GLExtension_texture_filter_anisotropic = true;

	GLBackend_LoadExtraFunctions();

	GETOPENGLFUNC(ClearDepthf)
	GETOPENGLFUNC(DepthRangef)

	Shader_LoadFunctions();

	return Shader_Compile();
}

boolean GLBackend_LoadExtraFunctions(void)
{
	GLExtension_LoadFunctions();

	GETOPENGLFUNC(VertexAttribPointer)

	GETOPENGLFUNCTRY(BlendEquation)
	GETOPENGLFUNCTRY(GenerateMipmap)

	if (pglGenerateMipmap)
		MipmapSupported = GL_TRUE;

	return true;
}

static void SetShader(int type)
{
	Shader_Set(GLBackend_GetShaderType(type));
}

static boolean CompileShaders(void)
{
	return Shader_Compile();
}

static void SetShaderInfo(hwdshaderinfo_t info, INT32 value)
{
	Shader_SetInfo(info, value);
}

static void LoadCustomShader(int number, char *shader, size_t size, boolean fragment)
{
	Shader_LoadCustom(number, shader, size, fragment);
}

static void UnSetShader(void)
{
	Shader_UnSet();
}

static void CleanShaders(void)
{
	Shader_Clean();
}

// -----------------+
// SetNoTexture     : Disable texture
// -----------------+
static void SetNoTexture(void)
{
	GLTexture_Disable();
}

static void GLPerspective(GLfloat fovy, GLfloat aspect)
{
	fmatrix4_t perspectiveMatrix;
	lzml_matrix4_perspective(perspectiveMatrix, Deg2Rad((float)fovy), (float)aspect, near_clipping_plane, FAR_CLIPPING_PLANE);
	lzml_matrix4_multiply(projMatrix, perspectiveMatrix);
}

static void GLProject(GLfloat objX, GLfloat objY, GLfloat objZ,
                      GLfloat* winX, GLfloat* winY, GLfloat* winZ)
{
	GLfloat in[4], out[4];
	int i;

	for (i=0; i<4; i++)
	{
		out[i] =
			objX * modelMatrix[0][i] +
			objY * modelMatrix[1][i] +
			objZ * modelMatrix[2][i] +
			modelMatrix[3][i];
	}
	for (i=0; i<4; i++)
	{
		in[i] =
			out[0] * projMatrix[0][i] +
			out[1] * projMatrix[1][i] +
			out[2] * projMatrix[2][i] +
			out[3] * projMatrix[3][i];
	}
	if (fpclassify(in[3]) == FP_ZERO) return;
	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];
	/* Map x, y and z to range 0-1 */
	in[0] = in[0] * 0.5f + 0.5f;
	in[1] = in[1] * 0.5f + 0.5f;
	in[2] = in[2] * 0.5f + 0.5f;

	/* Map x,y to viewport */
	in[0] = in[0] * viewport[2] + viewport[0];
	in[1] = in[1] * viewport[3] + viewport[1];

	*winX=in[0];
	*winY=in[1];
	*winZ=in[2];
}

static void FlushScreenTextures(void)
{
	GLTexture_FlushScreen();
}

// -----------------+
// SetModelView     :
// -----------------+
static void SetModelView(INT32 w, INT32 h)
{
	// The screen textures need to be flushed if the width or height change so that they be remade for the correct size
	if (screen_width != w || screen_height != h)
	{
		FlushScreenTextures();
#ifdef HAVE_GL_FRAMEBUFFER
		GLFramebuffer_DeleteAttachments();
#endif
	}

	screen_width = w;
	screen_height = h;

#ifdef HAVE_GL_FRAMEBUFFER
	RenderToFramebuffer = FramebufferEnabled;
	GLFramebuffer_Disable();

	if (RenderToFramebuffer)
	{
		pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		pglViewport(0, 0, w, h);
		GLFramebuffer_Enable();
	}
#endif

	pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pglViewport(0, 0, w, h);

	lzml_matrix4_identity(projMatrix);
	lzml_matrix4_identity(viewMatrix);
	lzml_matrix4_identity(modelMatrix);

	Shader_SetTransform();
}


// -----------------+
// SetBlend         : Set blend modes
// -----------------+
static void SetBlend(FBITFIELD PolyFlags)
{
	GLBackend_SetBlend(PolyFlags);
}


// -----------------+
// SetStates        : Set permanent states
// -----------------+
static void SetStates(void)
{
	alpha_threshold = 0.0f;

	pglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	pglEnable(GL_DEPTH_TEST);    // check the depth buffer
	pglDepthMask(GL_TRUE);             // enable writing to depth buffer
	pglClearDepthf(1.0f);
	pglDepthRangef(0.0f, 1.0f);
	pglDepthFunc(GL_LEQUAL);

	pglEnable(GL_BLEND);
	pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// this set CurrentPolyFlags to the actual configuration
	CurrentPolyFlags = 0xffffffff;
	SetBlend(0);

	if (gl_shaderstate.current)
	{
		Shader_EnableVertexAttribArray(LOC_POSITION);
		Shader_EnableVertexAttribArray(LOC_TEXCOORD);
		Shader_DisableVertexAttribArray(LOC_COLORS);
		Shader_DisableVertexAttribArray(LOC_NORMAL);
	}

	tex_downloaded = 0;
	SetNoTexture();
}

// -----------------+
// DeleteTexture    : Deletes a texture from the GPU and frees its data
// -----------------+
static void DeleteTexture(GLMipmap_t *pTexInfo)
{
	FTextureInfo *head = TexCacheHead;

	if (!pTexInfo)
		return;
	else if (pTexInfo->downloaded)
		pglDeleteTextures(1, (GLuint *)&pTexInfo->downloaded);

	while (head)
	{
		if (head->downloaded == pTexInfo->downloaded)
		{
			if (head->next)
				head->next->prev = head->prev;
			else // no next -> tail is being deleted -> update TexCacheTail
				TexCacheTail = head->prev;
			if (head->prev)
				head->prev->next = head->next;
			else // no prev -> head is being deleted -> update TexCacheHead
				TexCacheHead = head->next;
			free(head);
			break;
		}

		head = head->next;
	}

	pTexInfo->downloaded = 0;
}


// -----------------+
// Init             : Initialise the OpenGL ES interface API
// Returns          :
// -----------------+
static boolean Init(void)
{
	return GLBackend_Init();
}


// -----------------+
// RecreateContext  : Clears textures, buffer objects, and recompiles shaders.
// Returns          :
// -----------------+
static void RecreateContext(void)
{
	GLBackend_RecreateContext();
}


// -----------------+
// SetPalette       : Sets the current palette.
// Returns          :
// -----------------+
static void SetPalette(RGBA_t *palette)
{
	GLBackend_SetPalette(palette);
}


// -----------------+
// ClearMipMapCache : Flush OpenGL textures from memory
// -----------------+
static void ClearMipMapCache(void)
{
	GLTexture_Flush();
}


// -----------------+
// ReadRect         : Read a rectangle region of the truecolor framebuffer
//                  : store pixels as RGBA8888
// Returns          : RGBA8888 pixel array stored in dst_data
// -----------------+
static void ReadRect(INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT32 *dst_data)
{
	(void)dst_stride;
	GLBackend_ReadRectRGBA(x, y, width, height, dst_data);
}


// -----------------+
// GClipRect        : Defines the 2D hardware clipping window
// -----------------+
static void GClipRect(INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip)
{
	pglViewport(minx, screen_height-maxy, maxx-minx, maxy-miny);
	near_clipping_plane = nearclip;

	lzml_matrix4_identity(projMatrix);
	lzml_matrix4_identity(viewMatrix);
	lzml_matrix4_identity(modelMatrix);

	Shader_SetTransform();
}


// -----------------+
// ClearBuffer      : Clear the color/alpha/depth buffer(s)
// -----------------+
static void ClearBuffer(FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor)
{
	GLbitfield ClearMask = 0;

	if (ColorMask)
	{
		if (ClearColor)
			pglClearColor(ClearColor->red,
			              ClearColor->green,
			              ClearColor->blue,
			              ClearColor->alpha);
		ClearMask |= GL_COLOR_BUFFER_BIT;
	}
	if (DepthMask)
	{
		pglClearDepthf(1.0f);     //Hurdler: all that are permanen states
		pglDepthRangef(0.0f, 1.0f);
		pglDepthFunc(GL_LEQUAL);
		ClearMask |= GL_DEPTH_BUFFER_BIT;
	}

	SetBlend(DepthMask ? PF_Occlude | CurrentPolyFlags : CurrentPolyFlags&~PF_Occlude);

	pglClear(ClearMask);

	if (gl_shaderstate.current)
	{
		Shader_EnableVertexAttribArray(LOC_POSITION);
		Shader_EnableVertexAttribArray(LOC_TEXCOORD);
		Shader_DisableVertexAttribArray(LOC_COLORS);
		Shader_DisableVertexAttribArray(LOC_NORMAL);
	}
}


// -----------------+
// HWRAPI Draw2DLine: Render a 2D line
// -----------------+
static void Draw2DLine(F2DCoord *v1, F2DCoord *v2, RGBA_t Color)
{
	GLfloat p[12];
	GLfloat dx, dy;
	GLfloat angle;

	GLRGBAFloat fcolor = {Color.s.red/255.0f, Color.s.green/255.0f, Color.s.blue/255.0f, Color.s.alpha/255.0f};

	if (gl_shaderstate.current == NULL)
		return;

	SetNoTexture();

	// This is the preferred, 'modern' way of rendering lines -- creating a polygon.
	if (fabsf(v2->x - v1->x) > FLT_EPSILON)
		angle = (float)atan((v2->y-v1->y)/(v2->x-v1->x));
	else
		angle = (float)N_PI_DEMI;
	dx = (float)sin(angle) / (float)screen_width;
	dy = (float)cos(angle) / (float)screen_height;

	p[0] = v1->x - dx;  p[1] = -(v1->y + dy); p[2] = 1;
	p[3] = v2->x - dx;  p[4] = -(v2->y + dy); p[5] = 1;
	p[6] = v2->x + dx;  p[7] = -(v2->y - dy); p[8] = 1;
	p[9] = v1->x + dx;  p[10] = -(v1->y - dy); p[11] = 1;

	Shader_SetUniforms(NULL, &fcolor, NULL, NULL);

	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, p);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void SetClamp(UINT32 clamp)
{
	pglTexParameteri(GL_TEXTURE_2D, (GLenum)clamp, GL_CLAMP_TO_EDGE);
}

// -----------------+
// UpdateTexture    : Updates the texture data.
// -----------------+
static void UpdateTexture(GLMipmap_t *pTexInfo)
{
	// Upload a texture
	GLuint num = pTexInfo->downloaded;
	boolean update = true;

	INT32 w = pTexInfo->width, h = pTexInfo->height;
	INT32 i, j;

	const GLubyte *pImgData = (const GLubyte *)pTexInfo->data;
	const GLvoid *ptex = NULL;
	RGBA_t *tex = NULL;

	// Generate a new texture name.
	if (!num)
	{
		pglGenTextures(1, &num);
		pTexInfo->downloaded = num;
		update = false;
	}

	if (pTexInfo->format == GL_TEXFMT_P_8 || pTexInfo->format == GL_TEXFMT_AP_88)
	{
		GLTexture_AllocBuffer(pTexInfo);
		ptex = tex = TextureBuffer;

		for (j = 0; j < h; j++)
		{
			for (i = 0; i < w; i++)
			{
				if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					(pTexInfo->flags & TF_CHROMAKEYED))
				{
					tex[w*j+i].s.red   = 0;
					tex[w*j+i].s.green = 0;
					tex[w*j+i].s.blue  = 0;
					tex[w*j+i].s.alpha = 0;
					pTexInfo->flags |= TF_TRANSPARENT; // there is a hole in it
				}
				else
				{
					tex[w*j+i].s.red   = myPaletteData[*pImgData].s.red;
					tex[w*j+i].s.green = myPaletteData[*pImgData].s.green;
					tex[w*j+i].s.blue  = myPaletteData[*pImgData].s.blue;
					tex[w*j+i].s.alpha = myPaletteData[*pImgData].s.alpha;
				}

				pImgData++;

				if (pTexInfo->format == GL_TEXFMT_AP_88)
				{
					if (!(pTexInfo->flags & TF_CHROMAKEYED))
						tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}

			}
		}
	}
	else if (pTexInfo->format == GL_TEXFMT_RGBA)
	{
		// Directly upload the texture data
		ptex = pImgData;
	}
	else if (pTexInfo->format == GL_TEXFMT_ALPHA_INTENSITY_88)
	{
		GLTexture_AllocBuffer(pTexInfo);
		ptex = tex = TextureBuffer;

		for (j = 0; j < h; j++)
		{
			for (i = 0; i < w; i++)
			{
				tex[w*j+i].s.red   = *pImgData;
				tex[w*j+i].s.green = *pImgData;
				tex[w*j+i].s.blue  = *pImgData;
				pImgData++;
				tex[w*j+i].s.alpha = *pImgData;
				pImgData++;
			}
		}
	}
	else if (pTexInfo->format == GL_TEXFMT_ALPHA_8) // Used for fade masks
	{
		GLTexture_AllocBuffer(pTexInfo);
		ptex = tex = TextureBuffer;

		for (j = 0; j < h; j++)
		{
			for (i = 0; i < w; i++)
			{
				tex[w*j+i].s.red   = *pImgData;
				tex[w*j+i].s.green = 255;
				tex[w*j+i].s.blue  = 255;
				tex[w*j+i].s.alpha = 255;
				pImgData++;
			}
		}
	}

	pglBindTexture(GL_TEXTURE_2D, num);
	tex_downloaded = num;

	// disable texture filtering on any texture that has holes so there's no dumb borders or blending issues
	if (pTexInfo->flags & TF_TRANSPARENT)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	}

	if (pTexInfo->flags & TF_WRAPX)
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	else
		SetClamp(GL_TEXTURE_WRAP_S);

	if (pTexInfo->flags & TF_WRAPY)
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	else
		SetClamp(GL_TEXTURE_WRAP_T);

	if (GLExtension_texture_filter_anisotropic)
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic_filter);

	if (update)
		pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
	else
		pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);

	if (MipmapEnabled)
		pglGenerateMipmap(GL_TEXTURE_2D);
}

// -----------------+
// SetTexture       : The mipmap becomes the current texture source
// -----------------+
static void SetTexture(GLMipmap_t *pTexInfo)
{
	if (!pTexInfo)
	{
		SetNoTexture();
		return;
	}
	else if (pTexInfo->downloaded)
	{
		if (pTexInfo->downloaded != tex_downloaded)
		{
			pglBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);
			tex_downloaded = pTexInfo->downloaded;
		}
	}
	else
	{
		FTextureInfo *newTex = calloc(1, sizeof (*newTex));

		UpdateTexture(pTexInfo);

		newTex->texture = pTexInfo;
		newTex->downloaded = (UINT32)pTexInfo->downloaded;
		newTex->width = (UINT32)pTexInfo->width;
		newTex->height = (UINT32)pTexInfo->height;
		newTex->format = (UINT32)pTexInfo->format;

		// insertion at the tail
		if (TexCacheTail)
		{
			newTex->prev = TexCacheTail;
			TexCacheTail->next = newTex;
			TexCacheTail = newTex;
		}
		else // initialization of the linked list
			TexCacheTail = TexCacheHead = newTex;
	}
}

// code that is common between DrawPolygon and DrawIndexedTriangles
static void PreparePolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FBITFIELD PolyFlags, INT32 shader)
{
	GLRGBAFloat poly = {1.0f, 1.0f, 1.0f, 1.0f};
	GLRGBAFloat tint = {1.0f, 1.0f, 1.0f, 1.0f};
	GLRGBAFloat fade = {1.0f, 1.0f, 1.0f, 1.0f};

	GLRGBAFloat *c_poly = NULL, *c_tint = NULL, *c_fade = NULL;

	if (gl_shaderstate.current)
	{
		Shader_EnableVertexAttribArray(LOC_POSITION);
		Shader_EnableVertexAttribArray(LOC_TEXCOORD);
	}

	if (PolyFlags & PF_Corona)
		PolyFlags &= ~(PF_NoDepthTest|PF_Corona);

	SetBlend(PolyFlags);
	if (shader)
		SetShader(shader);

	// If Modulated, mix the surface colour to the texture
	if (pSurf)
	{
		// If Modulated, mix the surface colour to the texture
		if (CurrentPolyFlags & (PF_Modulated | PF_ColorMapped))
		{
			poly.red   = (pSurf->PolyColor.s.red/255.0f);
			poly.green = (pSurf->PolyColor.s.green/255.0f);
			poly.blue  = (pSurf->PolyColor.s.blue/255.0f);
			poly.alpha = (pSurf->PolyColor.s.alpha/255.0f);

			if (CurrentPolyFlags & PF_Modulated)
				c_poly = &poly;
			else
				c_poly = &white;
		}

		// Only if the surface is colormapped
		if (CurrentPolyFlags & PF_ColorMapped)
		{
			tint.red   = (pSurf->TintColor.s.red/255.0f);
			tint.green = (pSurf->TintColor.s.green/255.0f);
			tint.blue  = (pSurf->TintColor.s.blue/255.0f);
			tint.alpha = (pSurf->TintColor.s.alpha/255.0f);

			fade.red   = (pSurf->FadeColor.s.red/255.0f);
			fade.green = (pSurf->FadeColor.s.green/255.0f);
			fade.blue  = (pSurf->FadeColor.s.blue/255.0f);
			fade.alpha = (pSurf->FadeColor.s.alpha/255.0f);

			c_tint = &tint;
			c_fade = &fade;
		}
	}
	else
		c_poly = &white;

	Shader_SetUniforms(pSurf, c_poly, c_tint, c_fade);
}

// -----------------+
// DrawPolygon      : Render a polygon, set the texture, set render mode
// -----------------+
static void DrawPolygon_GLES2(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, INT32 shader)
{
	if (gl_shaderstate.current == NULL)
		return;

	PreparePolygon(pSurf, pOutVerts, PolyFlags, shader);

	pglBindBuffer(GL_ARRAY_BUFFER, 0);

	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(FOutVector), &pOutVerts[0].x);
	if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
		VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(FOutVector), &pOutVerts[0].s);

	pglDrawArrays(GL_TRIANGLE_FAN, 0, iNumPts);

	if (PolyFlags & PF_RemoveYWrap)
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (PolyFlags & PF_ForceWrapX)
		SetClamp(GL_TEXTURE_WRAP_S);

	if (PolyFlags & PF_ForceWrapY)
		SetClamp(GL_TEXTURE_WRAP_T);
}

static void DrawPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags)
{
	DrawPolygon_GLES2(pSurf, pOutVerts, iNumPts, PolyFlags, 0);
}

static void DrawPolygonShader(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, INT32 shader)
{
	DrawPolygon_GLES2(pSurf, pOutVerts, iNumPts, PolyFlags, shader);
}

static void DrawIndexedTriangles(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, UINT32 *IndexArray)
{
	if (gl_shaderstate.current == NULL)
		return;

	PreparePolygon(pSurf, pOutVerts, PolyFlags, 0);

	pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(FOutVector), &pOutVerts[0].x);
	if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
		VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(FOutVector), &pOutVerts[0].s);

	pglDrawElements(GL_TRIANGLES, iNumPts, GL_UNSIGNED_INT, IndexArray);

	// the DrawPolygon variant of this has some code about polyflags and wrapping here but havent noticed any problems from omitting it?
}

static void RenderSkyDome(gl_sky_t *sky)
{
	int i, j;

	fvector3_t scale;
	scale[0] = scale[2] = 1.0f;

	Shader_SetUniforms(NULL, NULL, NULL, NULL);

	// Build the sky dome! Yes!
	if (sky->rebuild)
	{
		// delete VBO when already exists
		if (sky->vbo)
			pglDeleteBuffers(1, &sky->vbo);

		// generate a new VBO and get the associated ID
		pglGenBuffers(1, &sky->vbo);

		// bind VBO in order to use
		pglBindBuffer(GL_ARRAY_BUFFER, sky->vbo);

		// upload data to VBO
		pglBufferData(GL_ARRAY_BUFFER, sky->vertex_count * sizeof(sky->data[0]), sky->data, GL_STATIC_DRAW);

		sky->rebuild = false;
	}

	if (gl_shaderstate.current == NULL)
	{
		pglBindBuffer(GL_ARRAY_BUFFER, 0);
		return;
	}

	// bind VBO in order to use
	pglBindBuffer(GL_ARRAY_BUFFER, sky->vbo);

	// activate and specify pointers to arrays
	Shader_EnableVertexAttribArray(LOC_COLORS);
	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(sky->data[0]), sky_vbo_x);
	if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
		VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(sky->data[0]), sky_vbo_u);
	VertexAttribPointer(LOC_COLORS, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(sky->data[0]), sky_vbo_r);

	// set transforms
	scale[1] = (float)sky->height / 200.0f;
	lzml_matrix4_scale(viewMatrix, scale);
	lzml_matrix4_rotate_y(viewMatrix, Deg2Rad(270.0f));

	Shader_SetTransform();

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < sky->loopcount; i++)
		{
			gl_skyloopdef_t *loop = &sky->loops[i];
			unsigned int mode = 0;

			if (j == 0 ? loop->use_texture : !loop->use_texture)
				continue;

			switch (loop->mode)
			{
				case HWD_SKYLOOP_FAN:
					mode = GL_TRIANGLE_FAN;
					break;
				case HWD_SKYLOOP_STRIP:
					mode = GL_TRIANGLE_STRIP;
					break;
				default:
					continue;
			}

			pglDrawArrays(mode, loop->vertexindex, loop->vertexcount);
		}
	}

	Shader_DisableVertexAttribArray(LOC_COLORS);

	// bind with 0, so, switch back to normal pointer operation
	pglBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void SetSpecialState(hwdspecialstate_t IdState, INT32 Value)
{
	switch (IdState)
	{
#ifdef HAVE_GL_FRAMEBUFFER
		case HWD_SET_FRAMEBUFFER:
			FramebufferEnabled = Value ? GL_TRUE : GL_FALSE;
			break;

		case HWD_SET_RENDERBUFFER_DEPTH:
			GLFramebuffer_SetDepth(Value);
			break;
#endif
		case HWD_SET_MODEL_LIGHTING:
			model_lighting = Value;
			break;

		case HWD_SET_SHADERS:
			gl_allowshaders = (hwdshaderoption_t)Value;
			break;

		case HWD_SET_TEXTUREFILTERMODE:
			GLTexture_SetFilterMode(Value);
			GLTexture_Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		case HWD_SET_TEXTUREANISOTROPICMODE:
			if (GLExtension_texture_filter_anisotropic)
			{
				anisotropic_filter = min(Value, maximumAnisotropy);
				GLTexture_Flush(); //??? if we want to change filter mode by texture, remove this
			}
			break;

		case HWD_SET_DITHER:
			if (Value)
				pglEnable(GL_DITHER);
			else
				pglDisable(GL_DITHER);

		default:
			break;
	}
}

static void CreateModelVBOs(model_t *model)
{
	GLModel_GenerateVBOs(model);
}

#define BUFFER_OFFSET(i) ((void*)(i))

// -----------------+
// HWRAPI DrawModel : Draw a model
// -----------------+
static void DrawModel(model_t *model, INT32 frameIndex, float duration, float tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface)
{
	static GLRGBAFloat poly = {1.0f, 1.0f, 1.0f, 1.0f};
	static GLRGBAFloat tint = {1.0f, 1.0f, 1.0f, 1.0f};
	static GLRGBAFloat fade = {1.0f, 1.0f, 1.0f, 1.0f};

	float pol = 0.0f;

	boolean useNormals;
	boolean useTinyFrames;
	boolean useVBO = true;

	fvector3_t v_scale;
	fvector3_t translate;

	FBITFIELD flags;
	int i;

	Shader_SetIfChanged(gl_shaderstate.current);

	Shader_EnableVertexAttribArray(LOC_TEXCOORD);
	Shader_EnableVertexAttribArray(LOC_POSITION);
	Shader_DisableVertexAttribArray(LOC_COLORS);

	if (gl_shaderstate.current == NULL)
		return;

	// Affect input model scaling
	scale *= 0.5f;
	v_scale[0] = v_scale[1] = v_scale[2] = scale;

	if (duration > 0.0 && tics >= 0.0) // don't interpolate if instantaneous or infinite in length
	{
		float newtime = (duration - tics); // + 1;

		pol = newtime / duration;

		if (pol > 1.0f)
			pol = 1.0f;

		if (pol < 0.0f)
			pol = 0.0f;
	}

	poly.red   = (Surface->PolyColor.s.red/255.0f);
	poly.green = (Surface->PolyColor.s.green/255.0f);
	poly.blue  = (Surface->PolyColor.s.blue/255.0f);
	poly.alpha = (Surface->PolyColor.s.alpha/255.0f);

	tint.red   = (Surface->TintColor.s.red/255.0f);
	tint.green = (Surface->TintColor.s.green/255.0f);
	tint.blue  = (Surface->TintColor.s.blue/255.0f);
	tint.alpha = (Surface->TintColor.s.alpha/255.0f);

	fade.red   = (Surface->FadeColor.s.red/255.0f);
	fade.green = (Surface->FadeColor.s.green/255.0f);
	fade.blue  = (Surface->FadeColor.s.blue/255.0f);
	fade.alpha = (Surface->FadeColor.s.alpha/255.0f);

	useNormals = model_lighting && (Shader_AttribLoc(LOC_NORMAL) != -1);
	if (useNormals)
		Shader_EnableVertexAttribArray(LOC_NORMAL);

	Shader_SetUniforms(Surface, &poly, &tint, &fade);

	pglEnable(GL_CULL_FACE);

#ifdef USE_FTRANSFORM_MIRROR
	// flipped is if the object is vertically flipped
	// hflipped is if the object is horizontally flipped
	// pos->flip is if the screen is flipped vertically
	// pos->mirror is if the screen is flipped horizontally
	// XOR all the flips together to figure out what culling to use!
	{
		boolean reversecull = (flipped ^ hflipped ^ pos->flip ^ pos->mirror);
		if (reversecull)
			pglCullFace(GL_FRONT);
		else
			pglCullFace(GL_BACK);
	}
#else
	// pos->flip is if the screen is flipped too
	if (flipped ^ hflipped ^ pos->flip) // If one or three of these are active, but not two, invert the model's culling
	{
		pglCullFace(GL_FRONT);
	}
	else
	{
		pglCullFace(GL_BACK);
	}
#endif

	lzml_matrix4_identity(modelMatrix);

	translate[0] = pos->x;
	translate[1] = pos->z;
	translate[2] = pos->y;
	lzml_matrix4_translate(modelMatrix, translate);

	if (flipped)
		v_scale[1] = -v_scale[1];
	if (hflipped)
		v_scale[2] = -v_scale[2];

	if (pos->roll)
	{
		float roll = (1.0f * pos->rollflip);
		fvector3_t rotate;

		translate[0] = pos->centerx;
		translate[1] = pos->centery;
		translate[2] = 0.0f;
		lzml_matrix4_translate(modelMatrix, translate);

		rotate[0] = rotate[1] = rotate[2] = 0.0f;

		if (pos->rotaxis == 2) // Z
			rotate[2] = roll;
		else if (pos->rotaxis == 1) // Y
			rotate[1] = roll;
		else // X
			rotate[0] = roll;

		lzml_matrix4_rotate_by_vector(modelMatrix, rotate, Deg2Rad(pos->rollangle));

		translate[0] = -translate[0];
		translate[1] = -translate[1];
		lzml_matrix4_translate(modelMatrix, translate);
	}

#ifdef USE_FTRANSFORM_ANGLEZ
	lzml_matrix4_rotate_z(modelMatrix, -Deg2Rad(pos->anglez)); // rotate by slope from Kart
#endif
	lzml_matrix4_rotate_y(modelMatrix, -Deg2Rad(pos->angley));
	lzml_matrix4_rotate_x(modelMatrix, Deg2Rad(pos->anglex));

	lzml_matrix4_scale(modelMatrix, v_scale);

	useTinyFrames = (model->meshes[0].tinyframes != NULL);
	if (useTinyFrames)
	{
		v_scale[0] = v_scale[1] = v_scale[2] = (1 / 64.0f);
		lzml_matrix4_scale(modelMatrix, v_scale);
	}

	// Don't use the VBO if it does not have the correct texture coordinates.
	// (Can happen when model uses a sprite as a texture and the sprite changes)
	// Comparing floats with the != operator here should be okay because they
	// are just copies of glpatches' max_s and max_t values.
	// Instead of the != operator, memcmp is used to avoid a compiler warning.
	if (memcmp(&(model->vbo_max_s), &(model->max_s), sizeof(model->max_s)) != 0 ||
		memcmp(&(model->vbo_max_t), &(model->max_t), sizeof(model->max_t)) != 0)
		useVBO = false;

	Shader_SetTransform();

	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (useTinyFrames)
		{
			tinyframe_t *frame = &mesh->tinyframes[frameIndex % mesh->numFrames];
			tinyframe_t *nextframe = NULL;

			if (nextFrameIndex != -1)
				nextframe = &mesh->tinyframes[nextFrameIndex % mesh->numFrames];

			if (!nextframe || fpclassify(pol) == FP_ZERO)
			{
				if (useVBO)
				{
					pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);

					VertexAttribPointer(LOC_POSITION, 3, GL_SHORT, GL_FALSE, sizeof(vbotiny_t), BUFFER_OFFSET(0));
					if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
						VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vbotiny_t), BUFFER_OFFSET(sizeof(short) * 3 + sizeof(char) * 6));
					if (useNormals)
						VertexAttribPointer(LOC_NORMAL, 3, GL_BYTE, GL_TRUE, sizeof(vbotiny_t), BUFFER_OFFSET(sizeof(short)*3));

					pglDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->indices);
					pglBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				else
				{
					VertexAttribPointer(LOC_POSITION, 3, GL_SHORT, GL_FALSE, 0, frame->vertices);
					if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
						VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, mesh->uvs);
					if (useNormals)
						VertexAttribPointer(LOC_NORMAL, 3, GL_BYTE, GL_TRUE, 0, frame->normals);

					pglDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->indices);
				}
			}
			else
			{
				short *vertPtr;
				char *normPtr;
				int j = 0;

				// Dangit, I soooo want to do this in a GLSL shader...
				GLModel_AllocLerpTinyBuffer(mesh->numVertices * sizeof(short) * 3);
				vertPtr = vertTinyBuffer;
				normPtr = normTinyBuffer;

				for (j = 0; j < mesh->numVertices * 3; j++)
				{
					// Interpolate
					*vertPtr++ = (short)(frame->vertices[j] + (pol * (nextframe->vertices[j] - frame->vertices[j])));
					*normPtr++ = (char)(frame->normals[j] + (pol * (nextframe->normals[j] - frame->normals[j])));
				}

				VertexAttribPointer(LOC_POSITION, 3, GL_SHORT, GL_FALSE, 0, vertTinyBuffer);
				if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
					VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, mesh->uvs);
				if (useNormals)
					VertexAttribPointer(LOC_NORMAL, 3, GL_BYTE, GL_TRUE, 0, normTinyBuffer);

				pglDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->indices);
			}
		}
		else
		{
			mdlframe_t *frame = &mesh->frames[frameIndex % mesh->numFrames];
			mdlframe_t *nextframe = NULL;

			if (nextFrameIndex != -1)
				nextframe = &mesh->frames[nextFrameIndex % mesh->numFrames];

			if (!nextframe || fpclassify(pol) == FP_ZERO)
			{
				if (useVBO)
				{
					// Zoom! Take advantage of just shoving the entire arrays to the GPU.
					pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);

					VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vbo64_t), BUFFER_OFFSET(0));
					if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
						VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vbo64_t), BUFFER_OFFSET(sizeof(float) * 6));
					if (useNormals)
						VertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vbo64_t), BUFFER_OFFSET(sizeof(float) * 3));

					pglDrawArrays(GL_TRIANGLES, 0, mesh->numTriangles * 3);
					pglBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				else
				{
					VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, frame->vertices);
					if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
						VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, mesh->uvs);
					if (useNormals)
						VertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, frame->normals);

					pglDrawArrays(GL_TRIANGLES, 0, mesh->numTriangles * 3);
				}
			}
			else
			{
				float *vertPtr;
				float *normPtr;
				int j = 0;

				// Dangit, I soooo want to do this in a GLSL shader...
				GLModel_AllocLerpBuffer(mesh->numVertices * sizeof(float) * 3);
				vertPtr = vertBuffer;
				normPtr = normBuffer;

				for (j = 0; j < mesh->numVertices * 3; j++)
				{
					// Interpolate
					*vertPtr++ = frame->vertices[j] + (pol * (nextframe->vertices[j] - frame->vertices[j]));
					*normPtr++ = frame->normals[j] + (pol * (nextframe->normals[j] - frame->normals[j]));
				}

				VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, vertBuffer);
				if (Shader_AttribLoc(LOC_TEXCOORD) != -1)
					VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, mesh->uvs);
				if (useNormals)
					VertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_TRUE, 0, normBuffer);

				pglDrawArrays(GL_TRIANGLES, 0, mesh->numVertices);
			}
		}
	}

	lzml_matrix4_identity(modelMatrix);
	Shader_SetTransform();

	if (useNormals)
		Shader_DisableVertexAttribArray(LOC_NORMAL);

	pglDisable(GL_CULL_FACE);
}

// -----------------+
// SetTransform     :
// -----------------+
static void SetTransform(FTransform *stransform)
{
	static boolean special_splitscreen;
	boolean shearing = false;
	float used_fov;

	fvector3_t scale;

	lzml_matrix4_identity(viewMatrix);
	lzml_matrix4_identity(modelMatrix);

	if (stransform)
	{
		used_fov = stransform->fovxangle;

#ifdef USE_FTRANSFORM_MIRROR
		// mirroring from Kart
		if (stransform->mirror)
		{
			scale[0] = -stransform->scalex;
			scale[1] = -stransform->scaley;
			scale[2] = -stransform->scalez;
		}
		else
#endif
		if (stransform->flip)
		{
			scale[0] = stransform->scalex;
			scale[1] = -stransform->scaley;
			scale[2] = -stransform->scalez;
		}
		else
		{
			scale[0] = stransform->scalex;
			scale[1] = stransform->scaley;
			scale[2] = -stransform->scalez;
		}

		lzml_matrix4_scale(viewMatrix, scale);

		if (stransform->roll)
			lzml_matrix4_rotate_z(viewMatrix, Deg2Rad(stransform->rollangle));
		lzml_matrix4_rotate_x(viewMatrix, Deg2Rad(stransform->anglex));
		lzml_matrix4_rotate_y(viewMatrix, Deg2Rad(stransform->angley + 270.0f));

		lzml_matrix4_translate_x(viewMatrix, -stransform->x);
		lzml_matrix4_translate_y(viewMatrix, -stransform->z);
		lzml_matrix4_translate_z(viewMatrix, -stransform->y);

		special_splitscreen = stransform->splitscreen;
		shearing = stransform->shearing;
	}
	else
		used_fov = fov;

	lzml_matrix4_identity(projMatrix);

	if (stransform)
	{
		// Simulate Software's y-shearing
		// https://zdoom.org/wiki/Y-shearing
		if (shearing)
		{
			float fdy = stransform->viewaiming * 2;
			if (stransform->flip)
				fdy *= -1.0f;
			lzml_matrix4_translate_y(projMatrix, (-fdy / BASEVIDHEIGHT));
		}

		if (special_splitscreen)
		{
			used_fov = (float)(atan(tan(used_fov*M_PI/360)*0.8)*360/M_PI);
			GLPerspective(used_fov, 2*ASPECT_RATIO);
		}
		else
			GLPerspective(used_fov, ASPECT_RATIO);
	}

	Shader_SetTransform();
}

static INT32 GetTextureUsed(void)
{
	return GLTexture_GetMemoryUsage(TexCacheHead);
}

static void PostImgRedraw(float points[SCREENVERTS][SCREENVERTS][2])
{
	INT32 x, y;
	float float_x, float_y, float_nextx, float_nexty;
	float xfix, yfix;
	INT32 texsize = 512;

	const float blackBack[16] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	if (gl_shaderstate.current == NULL)
		return;

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// X/Y stretch fix for all resolutions(!)
	xfix = (float)(texsize)/((float)((screen_width)/(float)(SCREENVERTS-1)));
	yfix = (float)(texsize)/((float)((screen_height)/(float)(SCREENVERTS-1)));

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_BLEND);

	Shader_DisableVertexAttribArray(LOC_TEXCOORD);

	// Draw a black square behind the screen texture,
	// so nothing shows through the edges
	Shader_SetUniforms(NULL, &black, NULL, NULL);
	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, blackBack);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	Shader_EnableVertexAttribArray(LOC_TEXCOORD);
	Shader_SetUniforms(NULL, &white, NULL, NULL);

	for(x=0;x<SCREENVERTS-1;x++)
	{
		for(y=0;y<SCREENVERTS-1;y++)
		{
			float stCoords[8];
			float vertCoords[12];

			// Used for texture coordinates
			// Annoying magic numbers to scale the square texture to
			// a non-square screen..
			float_x = (float)(x/(xfix));
			float_y = (float)(y/(yfix));
			float_nextx = (float)(x+1)/(xfix);
			float_nexty = (float)(y+1)/(yfix);

			// float stCoords[8];
			stCoords[0] = float_x;
			stCoords[1] = float_y;
			stCoords[2] = float_x;
			stCoords[3] = float_nexty;
			stCoords[4] = float_nextx;
			stCoords[5] = float_nexty;
			stCoords[6] = float_nextx;
			stCoords[7] = float_y;

			VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, stCoords);

			// float vertCoords[12];
			vertCoords[0] = points[x][y][0] / 4.5f;
			vertCoords[1] = points[x][y][1] / 4.5f;
			vertCoords[2] = 1.0f;
			vertCoords[3] = points[x][y + 1][0] / 4.5f;
			vertCoords[4] = points[x][y + 1][1] / 4.5f;
			vertCoords[5] = 1.0f;
			vertCoords[6] = points[x + 1][y + 1][0] / 4.5f;
			vertCoords[7] = points[x + 1][y + 1][1] / 4.5f;
			vertCoords[8] = 1.0f;
			vertCoords[9] = points[x + 1][y][0] / 4.5f;
			vertCoords[10] = points[x + 1][y][1] / 4.5f;
			vertCoords[11] = 1.0f;

			VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, vertCoords);

			pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	pglEnable(GL_DEPTH_TEST);
	pglEnable(GL_BLEND);
}

// Create Screen to fade from
static void StartScreenWipe(void)
{
	INT32 texsize = 512;
	boolean firstTime = (startScreenWipe == 0);

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		pglGenTextures(1, &startScreenWipe);
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		SetClamp(GL_TEXTURE_WRAP_S);
		SetClamp(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = startScreenWipe;
}

// Create Screen to fade to
static void EndScreenWipe(void)
{
	INT32 texsize = 512;
	boolean firstTime = (endScreenWipe == 0);

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		pglGenTextures(1, &endScreenWipe);
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		SetClamp(GL_TEXTURE_WRAP_S);
		SetClamp(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = endScreenWipe;
}


// Draw the last scene under the intermission
static void DrawIntermissionBG(void)
{
	INT32 texsize = 512;
	float xfix, yfix;

	const float screenVerts[12] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	float fix[8];

	if (gl_shaderstate.current == NULL)
		return;

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	// const float screenVerts[12]

	// float fix[8];
	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	pglBindTexture(GL_TEXTURE_2D, screentexture);
	Shader_SetUniforms(NULL, &white, NULL, NULL);

	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, screenVerts);
	VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, fix);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	tex_downloaded = screentexture;
}

// Do screen fades!
static void DoWipe(boolean tinted, boolean isfadingin, boolean istowhite)
{
	INT32 texsize = 512;
	float xfix, yfix;

	INT32 fademaskdownloaded = tex_downloaded; // the fade mask that has been set

	const float screenVerts[12] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	float fix[8];

	const float defaultST[8] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};

	if (gl_shaderstate.current == NULL)
		return;

	if (!GLExtension_multitexture)
		return;

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	// const float screenVerts[12]

	// float fix[8];
	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	SetBlend(PF_Modulated|PF_Translucent|PF_NoDepthTest);

	Shader_Set(tinted ? SHADER_FADEMASK_ADDITIVEANDSUBTRACTIVE : SHADER_FADEMASK);

	Shader_DisableVertexAttribArray(LOC_COLORS);
	Shader_EnableVertexAttribArray(LOC_TEXCOORD1);

	Shader_SetSampler(gluniform_startscreen, 0);
	Shader_SetSampler(gluniform_endscreen, 1);
	Shader_SetSampler(gluniform_fademask, 2);

	if (tinted)
	{
		Shader_SetIntegerUniform(gluniform_isfadingin, isfadingin);
		Shader_SetIntegerUniform(gluniform_istowhite, istowhite);
	}

	Shader_SetUniforms(NULL, &white, NULL, NULL);
	Shader_SetTransform();

	pglActiveTexture(GL_TEXTURE0 + 0);
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);
	pglActiveTexture(GL_TEXTURE0 + 1);
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);
	pglActiveTexture(GL_TEXTURE0 + 2);
	pglBindTexture(GL_TEXTURE_2D, fademaskdownloaded);

	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, screenVerts);
	VertexAttribPointer(LOC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, fix);
	VertexAttribPointer(LOC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, 0, defaultST);

	pglActiveTexture(GL_TEXTURE0);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	Shader_DisableVertexAttribArray(LOC_TEXCOORD1);

	UnSetShader();
	tex_downloaded = endScreenWipe;
}

static void DoScreenWipe(void)
{
	DoWipe(false, false, false);
}

static void DoTintedWipe(boolean isfadingin, boolean istowhite)
{
	DoWipe(true, isfadingin, istowhite);
}

// Create a texture from the screen.
static void MakeScreenTexture(void)
{
	INT32 texsize = 512;
	boolean firstTime = (screentexture == 0);

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		pglGenTextures(1, &screentexture);
	pglBindTexture(GL_TEXTURE_2D, screentexture);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		SetClamp(GL_TEXTURE_WRAP_S);
		SetClamp(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = screentexture;
}

static void MakeFinalScreenTexture(void)
{
	INT32 texsize = 512;
	boolean firstTime = (finalScreenTexture == 0);

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		pglGenTextures(1, &finalScreenTexture);
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		SetClamp(GL_TEXTURE_WRAP_S);
		SetClamp(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = finalScreenTexture;
}

static void DrawFinalScreenTexture(int width, int height)
{
	float xfix, yfix;
	float origaspect, newaspect;
	float xoff = 1, yoff = 1; // xoffset and yoffset for the polygon to have black bars around the screen
	FRGBAFloat clearColour;
	INT32 texsize = 512;

	float off[12];
	float fix[8];

	if (gl_shaderstate.current == NULL)
		return;

	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	origaspect = (float)screen_width / screen_height;
	newaspect = (float)width / height;
	if (origaspect < newaspect)
	{
		xoff = origaspect / newaspect;
		yoff = 1;
	}
	else if (origaspect > newaspect)
	{
		xoff = 1;
		yoff = newaspect / origaspect;
	}

	// float off[12];
	off[0] = -xoff;
	off[1] = -yoff;
	off[2] = 1.0f;
	off[3] = -xoff;
	off[4] = yoff;
	off[5] = 1.0f;
	off[6] = xoff;
	off[7] = yoff;
	off[8] = 1.0f;
	off[9] = xoff;
	off[10] = -yoff;
	off[11] = 1.0f;

	// float fix[8];
	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglViewport(0, 0, width, height);

	clearColour.red = clearColour.green = clearColour.blue = 0;
	clearColour.alpha = 1;
	ClearBuffer(true, false, &clearColour);
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);

	Shader_SetUniforms(NULL, &white, NULL, NULL);

	pglBindBuffer(GL_ARRAY_BUFFER, 0);
	VertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 0, off);
	VertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, fix);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	tex_downloaded = finalScreenTexture;
}

struct hwdriver_s GPU_API_OpenGLES = {
#define DEF(func) func,
	HWR_API_FUNCTIONS(DEF)
#undef DEF
};

#endif //HWRENDER
