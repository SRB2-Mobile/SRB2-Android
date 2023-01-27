// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_drv.h
/// \brief imports/exports for the 3D hardware low-level interface API

#ifndef __HWR_DRV_H__
#define __HWR_DRV_H__

// this must be here 19991024 by Kin
#include "../screen.h"
#include "hw_data.h"
#include "hw_defs.h"
#include "hw_md2.h"

// ==========================================================================
//                                                       STANDARD DLL EXPORTS
// ==========================================================================

#define API(fn) (*HWR_APIDef_ ## fn)

typedef boolean API(Init) (void);
typedef void API(RecreateContext) (void);
typedef void API(SetPalette) (RGBA_t *ppal);
typedef void API(FinishUpdate) (INT32 waitvbl);
typedef void API(Draw2DLine) (F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
typedef void API(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
typedef void API(DrawPolygonShader) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, INT32 shader);
typedef void API(DrawIndexedTriangles) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, UINT32 *IndexArray);
typedef void API(RenderSkyDome) (gl_sky_t *sky);
typedef void API(SetModelView) (INT32 w, INT32 h);
typedef void API(SetStates) (void);
typedef void API(SetBlend) (FBITFIELD PolyFlags);
typedef void API(SetNoTexture) (void);
typedef void API(SetClamp) (UINT32 clamp);
typedef void API(ClearBuffer) (FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
typedef void API(SetTexture) (GLMipmap_t *TexInfo);
typedef void API(UpdateTexture) (GLMipmap_t *TexInfo);
typedef void API(DeleteTexture) (GLMipmap_t *TexInfo);
typedef void API(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT32 *dst_data);
typedef void API(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip);
typedef void API(ClearMipMapCache) (void);
typedef void API(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value);
typedef void API(DrawModel) (model_t *model, INT32 frameIndex, float duration, float tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface);
typedef void API(CreateModelVBOs) (model_t *model);
typedef void API(SetTransform) (FTransform *ptransform);
typedef INT32 API(GetTextureUsed) (void);

typedef void API(FlushScreenTextures) (void);
typedef void API(StartScreenWipe) (void);
typedef void API(EndScreenWipe) (void);
typedef void API(DoScreenWipe) (void);
typedef void API(DoTintedWipe) (boolean isfadingin, boolean istowhite);
typedef void API(DrawIntermissionBG) (void);
typedef void API(MakeScreenTexture) (void);
typedef void API(MakeFinalScreenTexture) (void);
typedef void API(DrawFinalScreenTexture) (int width, int height);

#define SCREENVERTS 10
typedef void API(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2]);

typedef boolean API(CompileShaders) (void);
typedef void API(CleanShaders) (void);
typedef void API(SetShader) (int type);
typedef void API(UnSetShader) (void);

typedef void API(SetShaderInfo) (hwdshaderinfo_t info, INT32 value);
typedef void API(LoadCustomShader) (int number, char *code, size_t size, boolean isfragment);

// ==========================================================================
//                                      HWR DRIVER OBJECT, FOR CLIENT PROGRAM
// ==========================================================================

#undef API

#define HWR_API_FUNCTIONS(X)\
	X(Init)\
	X(RecreateContext)\
	X(SetPalette)\
	X(Draw2DLine)\
	X(DrawPolygon)\
	X(DrawPolygonShader)\
	X(DrawIndexedTriangles)\
	X(RenderSkyDome)\
	X(SetModelView)\
	X(SetStates)\
	X(SetBlend)\
	X(SetNoTexture)\
	X(SetClamp)\
	X(ClearBuffer)\
	X(SetTexture)\
	X(UpdateTexture)\
	X(DeleteTexture)\
	X(ReadRect)\
	X(GClipRect)\
	X(ClearMipMapCache)\
	X(SetSpecialState)\
	X(GetTextureUsed)\
	X(DrawModel)\
	X(CreateModelVBOs)\
	X(SetTransform)\
	X(PostImgRedraw)\
	X(FlushScreenTextures)\
	X(StartScreenWipe)\
	X(EndScreenWipe)\
	X(DoScreenWipe)\
	X(DoTintedWipe)\
	X(DrawIntermissionBG)\
	X(MakeScreenTexture)\
	X(MakeFinalScreenTexture)\
	X(DrawFinalScreenTexture)\
	X(CompileShaders)\
	X(CleanShaders)\
	X(SetShader)\
	X(UnSetShader)\
	X(SetShaderInfo)\
	X(LoadCustomShader)

#define HWD_FUNCS(func) HWR_APIDef_ ## func func;

struct hwdriver_s {
	HWR_API_FUNCTIONS(HWD_FUNCS)
};

#undef HWD_FUNCS

extern struct hwdriver_s *hwdriver;

extern struct hwdriver_s GPU_API_OpenGL;
extern struct hwdriver_s GPU_API_OpenGLES;

#define GPU hwdriver

#endif //__HWR_DRV_H__

