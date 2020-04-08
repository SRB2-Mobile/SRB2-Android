// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  w_handle.h
/// \brief WAD file handle

#include "doomtype.h"
#include <stdio.h>

#ifdef HAVE_SDL
#include "SDL.h"
#include "SDL_rwops.h"
#endif

#ifndef __W_HANDLE__
#define __W_HANDLE__

typedef enum
{
	FILEHANDLE_STANDARD, // stdlib handle
	FILEHANDLE_SDL,    // sdl rwops
} fhandletype_t;

#ifdef HAVE_WHANDLE
// WAD file handle
// This is read-only - WADs are not writable.
typedef struct filehandle_s
{
	void          *file;
	fhandletype_t  type;

	size_t      (*read)  (void *, void *, size_t, size_t);
	int         (*seek)  (void *, long int, int);
	long int    (*tell)  (void *);
	int         (*close) (void *);
	const char *(*error) (void *);

	void *lasterror;
} filehandle_t;

// File handle open / close
void       *File_Open(const char *filename, const char *filemode, fhandletype_t type);
int         File_Close(void *stream);

//
// Standard library file operations
//

// File read / seek / tell
size_t      File_StandardRead(void *f, void *ptr, size_t size, size_t count);
int         File_StandardSeek(void *stream, long int offset, int origin);
long int    File_StandardTell(void *stream);

// File close / error
int         File_StandardClose(void *f);
const char *File_StandardError(void *handle);

//
// SDL_RWops file operations
//

#ifdef HAVE_SDL
// File read / seek / tell
size_t      File_SDLRead(void *f, void *ptr, size_t size, size_t count);
int         File_SDLSeek(void *stream, long int offset, int origin);
long int    File_SDLTell(void *stream);

// File close / error
int         File_SDLClose(void *f);
const char *File_SDLError(void *handle);
#endif

// Macros for file operations
#define File_Read(ptr, size, count, fhandle) ((filehandle_t *)fhandle)->read(((filehandle_t *)fhandle), ptr, size, count)
#define File_Seek(fhandle, offset, origin)   ((filehandle_t *)fhandle)->seek(((filehandle_t *)fhandle), offset, origin)
#define File_Tell(fhandle)                   ((filehandle_t *)fhandle)->tell(((filehandle_t *)fhandle))
#define File_Error(fhandle)                  ((filehandle_t *)fhandle)->error(((filehandle_t *)fhandle))

#else

// If you don't need to use whandle.
#define File_Open(a, b, c) fopen(a, b)   // (const char *filename, const char *filemode, fhandletype_t type);
#define File_Read fread                  // (void *ptr, size_t size, size_t count, void *handle);
#define File_Seek fseek
#define File_Tell ftell
#define File_Close fclose
#define File_Error M_FileError

#endif // HAVE_WHANDLE

#endif
