// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023-2023 by Sonic Team Junior.
// Copyright (C) 2023 by SRB2 Mobile Project.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_textreader.c
/// \brief Text reader

#include "m_textreader.h"

#include "doomdef.h"
#include "z_zone.h"

textreader_t *TextReader_New(char *text, size_t size)
{
	textreader_t *r = ZZ_Calloc(sizeof(textreader_t));
	r->source = ZZ_Calloc(size + 1);
	r->current = r->source;
	r->size = size;
	r->line = 1;
	memcpy(r->source, text, size);
	return r;
}

void TextReader_Delete(textreader_t *r)
{
	Z_Free(r->source);
	Z_Free(r);
}

size_t TextReader_GetLine(textreader_t *r, char *buf, size_t n)
{
	size_t nlf = 0xFFFFFFFF;
	size_t ncr, length = 0;

	char *start = r->current;
	char *end = r->source + r->size;
	if (start >= end)
		return 0;

	char *lf = strpbrk(start, "\r\n");
	if (lf)
	{
		if (*lf == '\n')
			nlf = 1;
		else
			nlf = 0;

		length = lf - start;
		lf++;
	}
	else
		length = end - start;

	if (buf && n)
	{
		while (n > 1 && *start != '\0' && start < end)
		{
			*buf++ = *start++;
			n--;
		}

		if (n >= 1)
			*buf = '\0';
	}

	r->current += length;

	if (lf)
	{
		do
		{
			r->line += nlf;
			ncr = strspn(lf, "\r");
			lf += ncr;
			nlf = strspn(lf, "\n");
			lf += nlf;
		}
		while (nlf || ncr);

		r->current = lf;
	}

	return length;
}

size_t TextReader_GetLineLength(textreader_t *r)
{
	char *start = r->current;
	char *end = r->source + r->size;
	if (start >= end)
		return 0;

	char *lf = strpbrk(start, "\r\n");
	if (lf)
		return lf - start;
	else
		return end - start;
}
