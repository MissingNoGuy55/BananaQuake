/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/
// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/file.h>
#endif
#include <stdarg.h>

#ifdef NeXT
#include <libc.h>
#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)

// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strupr (char *in);
char *strlower (char *in);
#ifdef __linux__
int filelength (int handle);
#else
size_t filelength (FILE* handle);
#endif
//int tell (int handle);

double I_FloatTime (void);

void	Error (const char *error, ...);
int		CheckParm (const char *check);

#ifdef __linux__
int 	SafeOpenWrite (char *filename);
int 	SafeOpenRead (char *filename);
void 	SafeRead(int handle, void* buffer, long count);
void 	SafeWrite(int handle, void* buffer, long count);
#else
int 	SafeOpenWrite(char* filename);
int 	SafeOpenRead(char* filename);
void 	SafeRead(int handle, void* buffer, long count);
void 	SafeWrite(int handle, void* buffer, long count);
#endif
void 	*SafeMalloc (long size);

#ifdef __linux
long	LoadFile (char *filename, void **bufferptr);
void	SaveFile (char *filename, void *buffer, long count);
#else
long	LoadFile(char* filename, void** bufferptr);
void	SaveFile(char* filename, void* buffer, long count);
#endif

void 	DefaultExtension (char *path, const char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest);
void	ExtractFileExtension (char *path, char *dest);

long 	ParseNum (char *str);

short	BigShort (short l);
short	LittleShort (short l);
long	BigLong (long l);
long	LittleLong (long l);
float	BigFloat (float l);
float	LittleFloat (float l);

void PR_CheckEmptyString(const char* s);

char *COM_Parse (char *data);

extern	char	com_token[1024];
extern	int		com_eof;

#endif
