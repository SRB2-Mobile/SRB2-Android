// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hu_font.c
/// \brief Fonts

#include "hu_font.h"

#include "r_picformats.h"
#include "r_defs.h"

#include "v_video.h" // VID_BlitLinearScreen
#include "i_video.h" // rendermode

#include "w_wad.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hw_main.h" // HWR_MakePatch
#endif

//
// FONT LOADING
//

static void Font_LoadSource(font_t *font)
{
	UINT16 *pixels = NULL;

	lumpnum_t lumpnum = W_GetNumForName(font->source);
	UINT8 *lump = W_CacheLumpNum(lumpnum, PU_STATIC);
	size_t lumplength = W_LumpLength(lumpnum);

#ifndef NO_PNG_LUMPS
	if (Picture_IsLumpPNG(lump, lumplength))
	{
		INT32 pngwidth, pngheight;

		pixels = (UINT16 *)Picture_PNGConvert(lump, PICFMT_FLAT16, &pngwidth, &pngheight, NULL, NULL, lumplength, NULL, 0);

		font->width = pngwidth;
		font->height = pngheight;
	}
	else
#endif
	{
		patch_t *patch = W_CachePatchNum(lumpnum, PU_PATCH);

		font->width = patch->width;
		font->height = patch->height;

		pixels = (UINT16 *)Picture_Convert(PICFMT_PATCH, patch, PICFMT_FLAT16, 0, NULL, 0, 0, 0, 0, 0);
	}

	font->pixels = pixels;

	Z_Free(lump);
}

static UINT8 Font_GetPixel(font_t *font, INT32 x, INT32 y)
{
	UINT16 *pixels = font->pixels;
	return pixels[((y * font->width) + x)] & 0xFF;
}

static void Font_GetChars(font_t *font)
{
	INT32 x = 0, y = 0;
	INT32 w, h;
	INT32 tallest = 0;
	INT32 lastheight = 0;
	fontchar_t *chr;
	size_t numchars = 0;

	UINT8 borderpixel = Font_GetPixel(font, 0, 0);

	if (font->chars == NULL)
		font->chars = Z_Calloc(sizeof(fontchar_t), PU_HUDGFX, NULL);
	chr = font->chars;

	while (true)
	{
		boolean done = false, skip = false;
		INT32 starty;
		UINT8 px;

		w = 0;
		h = 0;

		// go down
		x++;
		y++;

		if (x >= font->width || y >= font->height)
			break;

		starty = y;

		// find height
		while (true)
		{
			if (y >= font->height)
			{
				// Don't care
				x = 0;
				y = (starty + lastheight);

				if (y >= font->height)
					done = true;
				else
				{
					skip = true;
					tallest = 0;
				}
				break;
			}

			px = Font_GetPixel(font, x, y);
			if (px == borderpixel)
			{
				y -= h;
				break;
			}

			y++;
			h++;
		}

		if (done)
			break;
		else if (h > tallest)
			tallest = h;

		// find width
		while (true)
		{
			// Don't care
			if (x >= font->width)
			{
				x = 0;
				y += lastheight;
				tallest = 0;
				skip = true;
				break;
			}

			px = Font_GetPixel(font, x, y);
			if (px == borderpixel)
				break;

			x++;
			w++;
		}

		if (done)
			break;
		else if (skip)
			continue;

		chr->x = (x - w);
		chr->y = y;
		chr->w = w;
		chr->h = h;

		lastheight = h;

		// go up
		y--;

		// no more characters in this line
		px = Font_GetPixel(font, x+1, y);
		if (px != borderpixel)
		{
			x = 0;
			y += (tallest + 1);
			tallest = 0;
		}

		// test if there can be a new char
		if (y+1 >= font->height)
			break;

		numchars++;
		if ((numchars + font->start) >= 0x7F)
			break;

		if (numchars >= font->size)
		{
			font->chars = Z_Realloc(font->chars, sizeof(fontchar_t) * numchars, PU_HUDGFX, NULL);
			font->size = numchars;
			chr = font->chars + (font->size - 1);
		}
		else
			chr++;
	}
}

static void Font_MakeChars(font_t *font)
{
	size_t c;

	for (c = 0; c < font->size; c++)
	{
		fontchar_t *chr = &font->chars[c];
		size_t size = (chr->w * chr->h * sizeof(UINT16));

		if (!size)
			continue;

		if (chr->pixels == NULL)
		{
			UINT16 *src = (UINT16 *)font->pixels + ((chr->y * font->width) + chr->x);
			UINT16 *dest = Z_Calloc(size, PU_HUDGFX, NULL);
			INT32 h = chr->h;

			chr->pixels = dest;

			while (h--)
			{
				M_Memcpy(dest, src, chr->w * sizeof(UINT16));
				dest += chr->w;
				src += font->width;
			}
		}

		if (chr->patch == NULL)
		{
			chr->patch = Picture_Convert(PICFMT_FLAT16, chr->pixels, PICFMT_PATCH, 0, NULL, chr->w, chr->h, 0, 0, 0);
			Z_Free(chr->pixels);
			chr->pixels = NULL;
		}
	}
}

font_t *Font_Load(const char *source, char start, size_t size)
{
	font_t *font = Z_Calloc(sizeof(font_t), PU_HUDGFX, NULL);

	font->source = Z_StrDup(source);
	font->start = start;
	font->size = size;

	if (size)
		font->chars = Z_Calloc(sizeof(fontchar_t) * size, PU_HUDGFX, NULL);

	Font_LoadSource(font);
	Font_GetChars(font);
	Font_MakeChars(font);

	return font;
}

//
// FONT DRAWING
//

static INT32 Font_GetDefaultWidth(font_t *font)
{
	if (font == NULL)
		return 0;
	return font->chars[0].w<<FRACBITS;
}

static INT32 Font_GetDefaultHeight(font_t *font)
{
	if (font == NULL)
		return 0;
	return font->chars[0].h<<FRACBITS;
}

void Font_DrawString(font_t *font, fixed_t x, fixed_t y, INT32 option, fixed_t scale, const char *string, const UINT8 *colormap)
{
	INT32 w, c, dupx, dupy, scrwidth;
	fixed_t cx = x, cy = y;
	const char *ch = string;
	fixed_t width = Font_GetDefaultWidth(font);
	fixed_t height = Font_GetDefaultHeight(font);
	fixed_t charhscale, charvscale;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (font == NULL || string == NULL)
		return;

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
	}
	else
		dupx = dupy = 1;

	switch (option & V_SCALEPATCHMASK)
	{
		case 1: // V_NOSCALEPATCH
			dupx = dupy = 1;
			break;
		case 2: // V_SMALLSCALEPATCH
			dupx = vid.smalldupx;
			dupy = vid.smalldupy;
			break;
		case 3: // V_MEDSCALEPATCH
			dupx = vid.meddupx;
			dupy = vid.meddupy;
			break;
		default:
			break;
	}

	charhscale = scale * dupx;
	charvscale = scale * dupy;
	scrwidth = vid.width / dupx;

	for (;;)
	{
		c = (*ch++) & 0x7f;
		if (!c)
			break;

		if (c == '\n')
		{
			cx = x;
			cy += FixedMul(height, charvscale);
			continue;
		}

		if (!lowercase)
			c = toupper(c);
		c -= font->start;
		if (c < 0 || c >= (signed)font->size || font->chars[c].patch == NULL)
		{
			cx += FixedMul(width>>1, charhscale);
			continue;
		}

		w = font->chars[c].w;
		if ((cx>>FRACBITS) > scrwidth)
			continue;

		V_DrawFixedPatch(cx, cy, scale, option, font->chars[c].patch, colormap);
		cx += FixedMul(w << FRACBITS, charhscale);
	}
}

void Font_GetStringSize(font_t *font, const char *string, INT32 option, fixed_t scale, INT32 *strwidth, INT32 *strheight)
{
	INT32 w, c;
	INT32 cx = 0, totalcx = 0;
	const char *ch = string;
	fixed_t width = Font_GetDefaultWidth(font);
	fixed_t height = Font_GetDefaultHeight(font);
	fixed_t totalheight = height;
	INT32 lowercase = (option & V_ALLOWLOWERCASE);

	if (font == NULL || string == NULL)
	{
		if (strwidth)
			*strwidth = 0;
		if (strheight)
			*strheight = 0;
		return;
	}

	for (;;)
	{
		c = (*ch++) & 0x7f;
		if (!c)
			break;

		if (c == '\n')
		{
			cx = 0;
			totalheight += FixedMul(height, scale);
			continue;
		}

		if (!lowercase)
			c = toupper(c);
		c -= font->start;
		if (c < 0 || c >= (signed)font->size || font->chars[c].patch == NULL)
		{
			cx += FixedMul(width>>1, scale);
			if (cx > totalcx)
				totalcx = cx;
			continue;
		}

		w = font->chars[c].w;
		cx += FixedMul(w << FRACBITS, scale);
		if (cx > totalcx)
			totalcx = cx;
	}

	if (strwidth)
		*strwidth = FixedInt(totalcx);
	if (strheight)
		*strheight = FixedInt(totalheight);
}
