// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  w_handle.c
/// \brief WAD file handle

#include "w_handle.h"
#include "w_wad.h"
#include "m_misc.h"

#include "i_system.h"
#include "console.h"

#ifdef HAVE_WHANDLE
// Open a file handle.
void *File_Open(const char *filename, const char *filemode, fhandletype_t type)
{
	filehandle_t *handle = calloc(sizeof(filehandle_t), 1);

	handle->type = type;

	if (type == FILEHANDLE_STANDARD)
	{
		CONS_Printf("Opening standard file\n");
		handle->read = &File_StandardRead;
		handle->seek = &File_StandardSeek;
		handle->tell = &File_StandardTell;
		handle->close = &File_StandardClose;
		handle->error = &File_StandardError;
		handle->file = fopen(filename, filemode);
	}
#ifdef HAVE_SDL
	else if (type == FILEHANDLE_SDL)
	{
		CONS_Printf("Opening SDL file\n");
		handle->read = &File_SDLRead;
		handle->seek = &File_SDLSeek;
		handle->tell = &File_SDLTell;
		handle->close = &File_SDLClose;
		handle->error = &File_SDLError;
		handle->file = SDL_RWFromFile(filename, filemode);
	}
#endif
	else
		I_Error("File_Open: unknown handle type!");

	return handle;
}

// Close the file handle.
int File_Close(void *f)
{
	filehandle_t *handle = (filehandle_t *)f;
	int ok = handle->close(handle->file);
	if (handle->lasterror)
		free(handle->lasterror);
	free(handle);
	return ok;
}

//
// Standard library file operations
//

// Read bytes from the stream into a buffer.
size_t File_StandardRead(void *f, void *ptr, size_t size, size_t count)
{
	filehandle_t *handle = (filehandle_t *)f;
	return fread(ptr, size, count, (FILE *)handle->file);
}

// Seek to the specified position in the stream.
int File_StandardSeek(void *f, long int offset, int origin)
{
	filehandle_t *handle = (filehandle_t *)f;
	return fseek((FILE *)handle->file, offset, origin);
}

// Get the current position in the stream.
long int File_StandardTell(void *f)
{
	filehandle_t *handle = (filehandle_t *)f;
	return ftell((FILE *)handle->file);
}

// Close the file handle.
int File_StandardClose(void *f)
{
	return fclose((FILE *)f);
}

// Get latest file error.
const char *File_StandardError(void *f)
{
	return M_FileError((FILE *)(((filehandle_t *)f)->file));
}

//
// SDL_RWops file operations
//

#ifdef HAVE_SDL
static void File_SDLSetError(filehandle_t *handle)
{
	const char *errorstring = SDL_GetError();
	if (handle->lasterror)
		free(handle->lasterror);
	handle->lasterror = calloc(strlen(errorstring) + 1, 1);
	strcpy(handle->lasterror, errorstring);
}

// Read bytes from the stream into a buffer.
size_t File_SDLRead(void *f, void *ptr, size_t size, size_t count)
{
	filehandle_t *handle = (filehandle_t *)f;
	size_t read = SDL_RWread((struct SDL_RWops *)handle->file, ptr, size, count);
	File_SDLSetError(handle);
	return read;
}

// Seek to the specified position in the stream.
int File_SDLSeek(void *f, long int offset, int origin)
{
	filehandle_t *handle = (filehandle_t *)f;
	int position, type = RW_SEEK_SET;

	// set seek type
	if (origin == SEEK_CUR)
		type = RW_SEEK_CUR;
	else if (origin == SEEK_END)
		type = RW_SEEK_END;

	position = (int)SDL_RWseek((struct SDL_RWops *)handle->file, offset, type);
	File_SDLSetError(handle);
	return position;
}

// Get the current position in the stream.
long int File_SDLTell(void *f)
{
	filehandle_t *handle = (filehandle_t *)f;
	long int position = (long int)SDL_RWtell((struct SDL_RWops *)handle->file);
	File_SDLSetError(handle);
	return position;
}

// Close the file handle.
int File_SDLClose(void *f)
{
	return SDL_RWclose((struct SDL_RWops *)f);
}

// Get latest file error.
const char *File_SDLError(void *f)
{
	return (const char *)(((filehandle_t *)f)->lasterror);
}

#endif // HAVE_SDL

#endif
