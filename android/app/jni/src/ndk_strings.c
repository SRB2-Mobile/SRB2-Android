// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ndk_strings.c
/// \brief Android string functions

#include "jni_android.h"
#include "ndk_strings.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char extsyms[1024];
static size_t numsyms;
static size_t appliedsyms;
static char symformat[1024];

static void resetsyms(void)
{
	numsyms = 0;
	appliedsyms = 0;
}

static void getsyms(char *str, const char *format)
{
	unsigned char *c;
	strncpy(symformat, format, 1024);
	for (c = (unsigned char *)symformat; *c; c++)
	{
		if (*c == 0x01 || *c >= 0x80)
		{
			extsyms[numsyms] = *c;
			*c = 0x01;
			numsyms++;
		}
	}
}

static void applysyms(char *str)
{
	unsigned char *c;
	for (c = (unsigned char *)str; *c; c++)
	{
		if (*c == 0x01)
		{
			*c = extsyms[appliedsyms];
			appliedsyms++;

			if (appliedsyms >= numsyms)
				break;
		}
	}
}

int Android_sprintf(char *str, const char *format, ...)
{
	va_list argptr;
	int ret;

	resetsyms();
	getsyms(str, format);

	va_start(argptr, format);
	ret = vsprintf(str, symformat, argptr);
	va_end(argptr);

	applysyms(str);

	return ret;
}

int Android_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list argptr;
	int ret;

	resetsyms();
	getsyms(str, format);

	va_start(argptr, format);
	ret = vsnprintf(str, size, symformat, argptr);
	va_end(argptr);

	applysyms(str);

	return ret;
}

///\brief Android vsnprintf errors on characters beyond 0x80 and makes me want to die.
// Lactozilla: Seems to happen with other string functions due to Bionic and vfprintf, left above \brief because it is funny
int Android_vsnprintf(char *str, size_t size, const char *format, va_list argptr)
{
	int ret;

	resetsyms();
	getsyms(str, format);

	ret = vsnprintf(str, size, symformat, argptr);

	applysyms(str);

	return ret;
}
