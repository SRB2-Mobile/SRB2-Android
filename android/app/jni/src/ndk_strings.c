// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
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

char *Android_strchr(const char *str, int character)
{
	const char *p = str;
	while (*p)
	{
		if (*p == (char)character)
			return ((char *)p);
		p++;
	}
	return NULL;
}

static unsigned char extsyms[8192];
static size_t numsyms;
static size_t appliedsyms;
static char symformat[8192];

static void resetsyms(void)
{
	numsyms = 0;
	appliedsyms = 0;
}

static void getsyms(char *str, const char *format)
{
	strncpy(symformat, format, sizeof symformat);

	for (unsigned char *c = (unsigned char *)symformat; *c; c++)
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
	for (unsigned char *c = (unsigned char *)str; *c; c++)
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

int Android_vsnprintf(char *str, size_t size, const char *format, va_list argptr)
{
	int ret;

	resetsyms();
	getsyms(str, format);

	ret = vsnprintf(str, size, symformat, argptr);

	applysyms(str);

	return ret;
}
