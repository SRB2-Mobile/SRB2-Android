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

#include "../../hardware/hw_data.h" // GLMipmap_s
#include "../../hardware/hw_defs.h" // FTextureInfo

#include <stdarg.h>

// ----------------------+
// GetTextureMemoryUsage : calculates total memory usage by textures
// Returns               : total memory amount used by textures, excluding mipmaps
// ----------------------+
INT32 GetTextureMemoryUsage(void *textures)
{
	FTextureInfo *head = (FTextureInfo *)textures;
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

// -----------------+
// isExtAvailable   : Look if an OpenGL extension is available
// Returns          : true if extension available
// -----------------+
INT32 isExtAvailable(const char *extension, const GLubyte *start)
{
	GLubyte         *where, *terminator;

	if (!extension || !start) return 0;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
		start = terminator;
	}
	return 0;
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
	CONS_Alert(CONS_WARNING, "%s", str);
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

void GL_MSG_Error(const char *format, ...)
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
