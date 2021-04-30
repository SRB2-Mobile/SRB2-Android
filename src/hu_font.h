// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hu_font.h
/// \brief Fonts

#ifndef __HU_FONT__
#define __HU_FONT__

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"
#include "r_defs.h" // patch_t

typedef struct
{
	INT32 x, y, w, h;
	void *pixels;
	patch_t *patch;
} fontchar_t;

typedef struct
{
	char start;
	size_t size;

	fontchar_t *chars;

	void *pixels;
	INT32 width, height;

	const char *source;
} font_t;

font_t *Font_Load(const char *source, char start, size_t size);

void Font_DrawString(font_t *font, fixed_t x, fixed_t y, INT32 option, fixed_t scale, const char *string, const UINT8 *colormap);
void Font_GetStringSize(font_t *font, const char *string, INT32 option, fixed_t scale, INT32 *strwidth, INT32 *strheight);

#endif
