// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief Main program, simply calls D_SRB2Main and D_SRB2Loop, the high level loop.

#if !defined(__ANDROID__)
#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"
#include "../m_misc.h"/* path shit */
#include "../i_system.h"

#if defined (__GNUC__) || defined (__unix__)
#include <unistd.h>
#endif

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#include <errno.h>
#endif

#ifdef HAVE_SDL

#ifdef HAVE_TTF
#include "SDL.h"
#include "i_ttf.h"
#endif

#if defined (_WIN32) && !defined (main)
//#define SDLMAIN
#endif

#ifdef SDLMAIN
#include "SDL_main.h"
#elif defined(FORCESDLMAIN)
extern int SDL_main(int argc, char *argv[]);
#endif

#ifndef DOXYGEN
#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0
#endif
#endif

#if defined (_WIN32)
#include "../win32/win_dbg.h"
typedef BOOL (WINAPI *p_IsDebuggerPresent)(VOID);
#endif


/**	\brief	The main function

	\param	argc	number of arg
	\param	*argv	string table

	\return	int
*/
#if defined (__GNUC__) && (__GNUC__ >= 4)
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif

#ifdef FORCESDLMAIN
int SDL_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	myargc = argc;
	myargv = argv; /// \todo pull out path to exe from this string

#ifdef HAVE_TTF
#ifdef _WIN32
	I_StartupTTF(FONTPOINTSIZE, SDL_INIT_VIDEO|SDL_INIT_AUDIO, SDL_SWSURFACE);
#else
	I_StartupTTF(FONTPOINTSIZE, SDL_INIT_VIDEO, SDL_SWSURFACE);
#endif
#endif

#ifdef LOGMESSAGES
	if (!M_CheckParm("-nolog"))
		I_InitLogging();
#endif/*LOGMESSAGES*/

	//I_OutputMsg("I_StartupSystem() ...\n");
	I_StartupSystem();
#if defined (_WIN32)
	{
#if 0 // just load the DLL
		p_IsDebuggerPresent pfnIsDebuggerPresent = (p_IsDebuggerPresent)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsDebuggerPresent");
		if ((!pfnIsDebuggerPresent || !pfnIsDebuggerPresent())
#ifdef BUGTRAP
			&& !InitBugTrap()
#endif
			)
#endif
		{
			LoadLibraryA("exchndl.dll");
		}
	}
#ifndef __MINGW32__
	prevExceptionFilter = SetUnhandledExceptionFilter(RecordExceptionInfo);
#endif
#endif

	// startup SRB2
	CONS_Printf("Setting up SRB2...\n");
	D_SRB2Main();
#ifdef LOGMESSAGES
	if (!M_CheckParm("-nolog"))
		CONS_Printf("Logfile: %s\n", logfilename);
#endif
	CONS_Printf("Entering main game loop...\n");
	// never return
	D_SRB2Loop();

#ifdef BUGTRAP
	// This is safe even if BT didn't start.
	ShutdownBugTrap();
#endif

	// return to OS
	return 0;
}
#endif
#endif
