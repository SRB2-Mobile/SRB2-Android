// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2020-2021 by Jaime Ita Passos.
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
	FILEHANDLE_SDL,      // sdl rwops
} fhandletype_t;

#ifdef HAVE_WHANDLE
// WAD file handle
// This is read-only - WADs are not writable.
typedef struct filehandle_s
{
	void          *file;
	fhandletype_t  type;

	size_t      (*read)      (void *, void *, size_t, size_t);
	int         (*seek)      (void *, long int, int);
	long int    (*tell)      (void *);

	int         (*getchar)   (void *);
	char       *(*getstring) (void *, char *, int);

	int         (*close)     (void *);
	const char *(*error)     (void *);
	int         (*eof)       (void *);

	void *lasterror;
} filehandle_t;

// File handle open / close / error
void       *File_Open(const char *filename, const char *filemode, fhandletype_t type);
int         File_Close(void *stream);
int         File_CheckError(void *stream);

// Macros for file operations
#define File_Read(ptr, size, count, fhandle) ((filehandle_t *)fhandle)->read(((filehandle_t *)fhandle), ptr, size, count)
#define File_Seek(fhandle, offset, origin)   ((filehandle_t *)fhandle)->seek(((filehandle_t *)fhandle), offset, origin)
#define File_Tell(fhandle)                   ((filehandle_t *)fhandle)->tell(((filehandle_t *)fhandle))
#define File_GetChar(fhandle)                ((filehandle_t *)fhandle)->getchar(((filehandle_t *)fhandle))
#define File_GetString(str, num, fhandle)    ((filehandle_t *)fhandle)->getstring(((filehandle_t *)fhandle), str, num)
#define File_Error(fhandle)                  ((filehandle_t *)fhandle)->error(((filehandle_t *)fhandle))
#define File_EOF(fhandle)                    ((filehandle_t *)fhandle)->eof(((filehandle_t *)fhandle))

#else

// If you don't need to use whandle.
#define File_Open(fname, fmode, ftype) fopen(fname, fmode)
#define File_Read fread
#define File_Seek fseek
#define File_Tell ftell
#define File_GetChar fgetc
#define File_GetString fgets
#define File_CheckError ferror
#define File_Close fclose
#define File_Error M_FileError
#define File_EOF feof

#endif // HAVE_WHANDLE

#endif
