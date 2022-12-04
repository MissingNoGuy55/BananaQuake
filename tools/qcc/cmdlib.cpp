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
// cmdlib.c

#include "cmdlib.h"
#include <time.h>
#include <sysinfoapi.h>

#define PATHSEPERATOR   '/'

// set these before calling CheckParm
int myargc;
char **myargv;

char	com_token[1024];
int		com_eof;

/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	/*struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;*/

	LPSYSTEMTIME systime = {};

	GetSystemTime(systime);

	return (systime->wSecond / 1000000.0f);


}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int		c;
	int		len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			com_eof = true;
			return NULL;			// end of file;
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c=='\"')
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		} while (1);
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}




/*
================
filelength
================
*/
#if 0	// Missi: already defined through common.cpp (12/3/2022)
#ifdef __linux
int filelength (int handle)
{
	struct stat	fileinfo;
    
	if (fstat (handle,&fileinfo) == -1)
	{
		Error ("Error fstating");
	}

	return fileinfo.st_size;
}
#elif _WIN32
int filelength(FILE* handle)
{
	struct stat	fileinfo;
	LPDWORD size = 0;

	if (GetFileSize(handle, size) == -1)
	{
		Error("Error fstating");
	}

	fileinfo.st_size = (off_t)size;

	return fileinfo.st_size;
}
#endif
#endif

int tell (FILE* handle)
{
	return fseek (handle, 0, SEEK_CUR);
}

//char *strupr (char *start)
//{
//	char	*in;
//	in = start;
//	while (*in)
//	{
//		*in = toupper(*in);
//		in++;
//	}
//	return start;
//}

char *strlower (char *start)
{
	char	*in;
	in = start;
	while (*in)
	{
		*in = tolower(*in);
		in++;
	}
	return start;
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

/*
=================
Error

For abnormal program terminations
=================
*/
void Error (const char *error, ...)
{
	va_list argptr;

	printf ("\n************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (const char *check)
{
	int             i;

	for (i = 1;i<myargc;i++)
	{
		if ( !_strcmpi(check, myargv[i]) )
			return i;
	}

	return 0;
}


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __linux
int SafeOpenWrite (char *filename)
#elif _WIN32
FILE* SafeOpenWrite(const char* filename)
#endif
{
#ifdef __linux
	int     handle;
	umask (0);
	
	handle = open(filename,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY
	, 0666);
#else

	FILE* handle;

	if (fopen_s(&handle, filename, "wb") != 0)
		Error("Error opening %s: %s", filename, strerror(errno));

#endif

	return handle;
}

#ifdef __linux
int SafeOpenRead (const char *filename)
#elif _WIN32
FILE* SafeOpenRead (const char *filename)
#endif
{
#ifdef __linux
	int     handle;
	handle = open(filename,O_RDONLY | O_BINARY);

	if (handle == -1)
		Error ("Error opening %s: %s",filename,strerror(errno));
#elif _WIN32
	FILE*     handle;
	handle = fopen(filename, "rb");

	if (handle == NULL)
		Error("Error opening %s: %s", filename, strerror(errno));
#endif
	return handle;
}

#ifdef __linux
void SafeRead (int handle, void *buffer, long count)
{
	if (read (handle,buffer,count) != count)
		Error ("File read failure");
}


void SafeWrite (int handle, void *buffer, long count)
{
	if (write (handle,buffer,count) != count)
		Error ("File write failure");
}
#elif _WIN32
void SafeRead(FILE* handle, void* buffer, long count)
{
#if 0
	if (fread(buffer, 1, count, handle) != count)
		Error("File read failure");
#else

	size_t len = 0;

	len = fread_s(buffer, count, sizeof(char), count, handle);

	if (len == 0)
		Error("File read failure");

#endif

}


void SafeWrite(FILE* handle, void* buffer, long count)
{
	if (fwrite(buffer, sizeof(char), count, handle) != count)
		Error("File write failure");
}
#endif


void *SafeMalloc (long size)
{
	void *ptr;

	ptr = malloc (size);

	if (!ptr)
		Error ("Malloc failure for %lu bytes",size);

	return ptr;
}


/*
==============
LoadFile
==============
*/
void*    LoadFile (const char *filename, void *bufferptr)
{
#ifdef __linux
	int     handle;
	long    length;
	void    *buffer;
#elif _WIN32
	FILE*   handle;
	DWORD    length;
	void* buffer;
#endif

	handle = SafeOpenRead (filename);
	length = filelength (handle);

	buffer = calloc(1, length);


#ifdef __linux
	buffer = SafeMalloc(length + 1);
	((byte*)buffer)[length] = 0;
#elif _WIN32
	((byte*)buffer)[length] = 0;
#endif



	SafeRead (handle, buffer, length);
#ifdef __linux
	close(handle);
#elif _WIN32
	fclose(handle);
#endif

	return buffer;
}


/*
==============
SaveFile
==============
*/
#ifdef __linux
void    SaveFile (const char *filename, void *buffer, long count)
{
	int             handle;

	handle = SafeOpenWrite (filename);
	SafeWrite (handle, buffer, count);
	close (handle);
}
#elif _WIN32
void    SaveFile(const char* filename, FILE* buffer, long count)
{
	FILE*             handle;

	handle = SafeOpenWrite(filename);
	SafeWrite(handle, buffer, count);
	fclose(handle);
}
#endif



void DefaultExtension (char *path, const char *extension)
{
	const char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

void    StripExtension (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
void ExtractFilePath (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void ExtractFileExtension (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}


/*
==============
ParseNum / ParseHex
==============
*/
long ParseHex (char *hex)
{
	char    *str;
	long    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: %s",hex);
		str++;
	}

	return num;
}


long ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}



/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   BigShort (short l)
{
	return l;
}


long    LittleLong (long l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((long)b1<<24) + ((long)b2<<16) + ((long)b3<<8) + b4;
}

long    BigLong (long l)
{
	return l;
}


float	LittleFloat (float l)
{
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	BigFloat (float l)
{
	return l;
}


#else


short   BigShort (short l)
{
	unsigned char    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   LittleShort (short l)
{
	return l;
}


long    BigLong (long l)
{
	unsigned char    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((long)b1<<24) + ((long)b2<<16) + ((long)b3<<8) + b4;
}

long    LittleLong (long l)
{
	return l;
}

float	BigFloat (float l)
{
	union { unsigned char b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	LittleFloat (float l)
{
	return l;
}



#endif

