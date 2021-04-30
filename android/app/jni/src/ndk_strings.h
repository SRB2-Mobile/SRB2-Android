// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ndk_strings.h
/// \brief Android string functions

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *Android_strchr(const char *str, int character);

int Android_sprintf(char *str, const char *format, ...);
int Android_snprintf(char *str, size_t size, const char *format, ...);
int Android_vsnprintf(char *str, size_t size, const char *format, va_list argptr);
