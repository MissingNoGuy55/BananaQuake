/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// comndef.h  -- general definitions

#ifndef COMMON_H
#define COMMON_H

#include "sys.h"
#include "vpkfile.h"

#if defined(__linux__) || defined(__CYGWIN__)
#define _strdup strdup
#define _stricmp stricmp
#define _strnicmp strnicmp
#elif defined(_WIN32)
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#endif

#if !defined BYTE_DEFINED
typedef unsigned char byte;
#define BYTE_DEFINED 1
#endif

//============================================================================

typedef struct sizebuf_s
{
	bool	allowoverflow;	// if false, do a Sys_Error
	bool	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize);
void SZ_Free (sizebuf_t *buf);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!

#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (size_t)&(((t *)0)->m)))     // Missi: changed to size_t (4/22/2023)

//============================================================================

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((float)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((float)0x7fffffff)

// Missi: copied from QuakeSpasm (2/11/22)
#define	q_min(a, b)	(((a) < (b)) ? (a) : (b))
#define	q_max(a, b)	(((a) > (b)) ? (a) : (b))
#define	CLAMP(_minval, x, _maxval)		\
	((x) < (_minval) ? (_minval) :		\
	 (x) > (_maxval) ? (_maxval) : (x))

// Missi: to get rid of the magic numbers (5/30/2022)
enum HunkTypes_e
{
	HUNK_ZONE = 0,
	HUNK_FULL = 1,
	HUNK_TEMP = 2,
	HUNK_CACHE = 3,
	HUNK_TEMP_FILL = 4
};

//============================================================================

extern	bool		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int		(*BigLong) (int l);
extern	int		(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);

extern	int			msg_readcount;
extern	bool	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
const char *MSG_ReadString (void);

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);

//============================================================================

void Q_memset (void *dest, int fill, size_t count);
void Q_memcpy (void *dest, const void *src, size_t count);
int Q_memcmp (void *m1, void *m2, size_t count);
void Q_strcpy (char *dest, const char *src);
void Q_strncpy (char *dest, const char *src, size_t count);
int Q_strlen (const char *str);
char *Q_strrchr (const char *s, char c);
void Q_strcat (char *dest, const char *src);
int Q_strcmp (const char *s1, const char *s2);
int Q_strncmp (const char *s1, const char *s2, size_t count);
int Q_strcasecmp (const char *s1, const char *s2);
int Q_strncasecmp (const char *s1, const char *s2, size_t n);
int	Q_atoi (const char *str);
float Q_atof (const char *str);
int Q_vsnprintf_s(char* str, size_t size, size_t len, const char* format, va_list args);

void Q_FixSlashes(char* str, size_t size, const char delimiter = '\\');
void Q_FixQuotes(char* dest, const char* src, size_t size);
//============================================================================

//
// in memory
//

typedef struct
{
	char    name[MAX_QPATH];
	int             filepos, filelen;
} packfile_t;

typedef struct pack_s
{
	char    filename[MAX_OSPATH];
	int             handle;
	int             numfiles;
	packfile_t* files;
} pack_t;

//
// on disk
//
typedef struct
{
	char    name[56];
	int             filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int             dirofs;
	int             dirlen;
} dpackheader_t;

void COM_Path_f();

typedef struct _fshandle_t
{
	FILE* file;
	bool pak;	/* is the file read from a pak */
	long start;	/* file or data start position */
	long length;	/* file or data size */
	long pos;	/* current position relative to start */
} fshandle_t;

class CCommon
{
public:

	const char* COM_Parse(const char* data);
	const char* COM_ParseIntNewline(const char* buffer, int* value);
	const char* COM_ParseFloatNewline(const char* buffer, float* value);
	const char* COM_ParseStringNewline(const char* buffer);
    const int COM_ParseStringLength(const char* buffer, size_t len) const;

	int COM_CheckParm (const char *parm);
	void COM_Init (const char *path);
	void COM_CheckRegistered(void);
	void COM_InitArgv (int argc, char **argv);

	void COM_AddExtension(char* path, const char* extension, size_t len);	// Missi: copied from QuakeSpasm (1/10/2023)

	char* COM_SkipPath (const char *pathname);
	void COM_StripExtension (const char *in, char *out);
	void COM_FileBase(const char* in, char* out, size_t outsize);
	void COM_DefaultExtension (char *path, const char *extension);
	const char* COM_FileGetExtension(const char* in);
	bool COM_FileExists(const char* filename, uintptr_t* path_id);
	static void COM_CopyFile(const char* netpath, char* cachepath);
	void COM_WriteFile (const char *filename, void *data, int len);

	static void COM_CreatePath(const char* path);
	int COM_OpenFile (const char *filename, int *handle, uintptr_t* path_id);
	static int COM_FOpenFile (const char *filename, FILE **file, uintptr_t* path_id);
    int COM_FOpenFile_IFStream(const char* filename, cxxifstream* file, uintptr_t* path_id);
    void COM_FOpenFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id);
	cxxifstream* COM_FOpenFile_VPK(const char* filename, uintptr_t* path_id);
	void COM_CloseFile (int h);

	byte* COM_LoadMallocFile_TextMode_OSPath(const char* path, long* len_out);

	void COM_AddGameDirectory(const char* dir);

	void COM_InitFilesystem();
	static int COM_FindFile(const char* filename, int* handle, FILE** file, uintptr_t* path_id);
    int COM_FindFile_IFStream(const char* filename, int* handle, cxxifstream* file, uintptr_t* path_id);
    void COM_FindFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id);
    cxxifstream* COM_FindFile_VPK(const char* filename, uintptr_t* path_id);
	static long COM_filelength(FILE* f);
    size_t COM_filelength_FStream(cxxifstream* f);
	pack_t* COM_LoadPackFile(char* packfile);

	static void COM_Path_f(void);

	// Missi: buffer-safe varargs (4/30/2023)
	const char	*va(const char *format, ...);

	// does a varargs printf into a temp buffer
	// Missi: made into va_unsafe from va for backwards compatibility (4/30/2023)
	char* va_unsafe(char* format, ...);

    static	char	com_token[1024];
	static	bool	com_eof;

	static	int		com_argc;
	static	char	**com_argv;

	static	int		com_filesize;
	static	char	com_gamedir[MAX_OSPATH];
	static	char	com_cachedir[MAX_OSPATH];
	static	int		file_from_pak;

};

extern CCommon* g_Common;

#define	FS_ENT_NONE			(0)
#define	FS_ENT_FILE			(1 << 0)
#define	FS_ENT_DIRECTORY	(1 << 1)

// Missi: Repurposed QuakeSpasm filesystem stuff (1/1/2023)
class CFileSystem
{
public:
	static size_t FS_fread(void* ptr, size_t size, size_t nmemb, fshandle_t* fh);
	int FS_fseek(fshandle_t* fh, long offset, int whence);
	static long FS_ftell(fshandle_t* fh);
	void FS_rewind(fshandle_t* fh);
	int FS_feof(fshandle_t* fh);
	int FS_ferror(fshandle_t* fh);
	int FS_fclose(fshandle_t* fh);
	int FS_fgetc(fshandle_t* fh);
	char* FS_fgets(char* s, int size, fshandle_t* fh);
	long FS_filelength(fshandle_t* fh);
};

extern CFileSystem* g_FileSystem;

//============================================================================


template<typename T>
int loadsize;

template<typename T>
T* loadbuf;

template<typename T>
T* COM_LoadStackFile (const char *path, void* buffer, int bufsize, uintptr_t* path_id);

template<typename T>
T* COM_LoadStackFile_VPK (const char *path, void* buffer, int bufsize, uintptr_t* path_id);

template<typename T>
T* COM_LoadTempFile (const char *path, uintptr_t* path_id);

template<typename T>
void COM_LoadCacheFile (const char *path, struct cache_user_s *cu, uintptr_t* path_id);

extern struct cvar_s	registered;
extern bool		standard_quake, rogue, hipnotic;
extern cache_user_s* loadcache;

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.	
============
*/

#include "zone.h"

template<typename T>
inline T* COM_LoadFile(const char* path, int usehunk, uintptr_t* path_id);

template<typename T>
inline T* COM_LoadFile_VPK(const char* path, int usehunk, uintptr_t* path_id);

template<typename T>
inline T* COM_LoadFile(const char* path, int usehunk, uintptr_t* path_id)
{
	int	 h				= 0;
	T* buf				= nullptr;
	char base[32]		= {};
	int	len				= 0;

	buf = nullptr;     // quiet compiler warning

	// look for it in the filesystem or pack files
	len = g_Common->COM_OpenFile(path, &h, path_id);
	if (h == -1)
		return NULL;

	// extract the filename base name for hunk tag
	g_Common->COM_FileBase(path, base, len);

	switch (usehunk)
	{
	case HUNK_FULL:
		buf = static_cast<T*>(g_MemCache->Hunk_AllocName<T>(len + 1, base));
		break;
	case HUNK_TEMP:
		buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
		break;
	case HUNK_ZONE:
		buf = static_cast<T*>(mainzone->Z_Malloc<T>(len + 1));
		break;
	case HUNK_CACHE:
		buf = static_cast<T*>(g_MemCache->Cache_Alloc<T>(loadcache, len + 1, base));
		break;
	case HUNK_TEMP_FILL:
		if (len + 1 > loadsize<T>)
		{
			buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
			break;
		}
		else
		{
			buf = loadbuf<T>;
			break;
		}

	default:

		Sys_Error("COM_LoadFile: bad usehunk");
		break;

	}

	if (!buf)
		Sys_Error("COM_LoadFile: not enough space for %s", path);
	else
		((byte*)buf)[len] = 0;

#if 0
#ifndef GLQUAKE
	g_SoftwareRenderer->Draw_BeginDisc();
#else
	g_GLRenderer->Draw_BeginDisc();
#endif
#endif

	Sys_FileRead(h, buf, len);
	g_Common->COM_CloseFile(h);

#if 0
#ifndef GLQUAKE
	g_SoftwareRenderer->Draw_EndDisc();
#else
	g_GLRenderer->Draw_EndDisc();
#endif
#endif

	return buf;
}

template<typename T>
inline T* COM_LoadFile_VPK(const char* path, int usehunk, uintptr_t* path_id)
{
	cxxifstream* f = nullptr;
	T* buf = nullptr;
	char base[32] = {};
	int	len = 0;

	// look for it in VPK files
	f = g_Common->COM_FOpenFile_VPK(path, path_id);
	
	if (!f || f->bad() || len < 0)
	{
		delete f;
		f = nullptr;
		return nullptr;
	}

	len = g_Common->com_filesize;

	// extract the filename base name for hunk tag
	g_Common->COM_FileBase(path, base, len);

	switch (usehunk)
	{
	case HUNK_FULL:
		buf = static_cast<T*>(g_MemCache->Hunk_AllocName<T>(len + 1, base));
		break;
	case HUNK_TEMP:
		buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
		break;
	case HUNK_ZONE:
		buf = static_cast<T*>(mainzone->Z_Malloc<T>(len + 1));
		break;
	case HUNK_CACHE:
		buf = static_cast<T*>(g_MemCache->Cache_Alloc<T>(loadcache, len + 1, base));
		break;
	case HUNK_TEMP_FILL:
		if (len + 1 > loadsize<T>)
		{
			buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
			break;
		}
		else
		{
			buf = loadbuf<T>;
			break;
		}

	default:
		Sys_Error("COM_LoadFile: bad usehunk");
		break;
	}

	if (!buf)
		Sys_Error("COM_LoadFile: not enough space for %s", path);
	else
		((byte*)buf)[len] = 0;

	f->read(buf, len);
	f->seekg(0, cxxifstream::beg);

	return buf;
}

template<typename T>
inline T* COM_LoadFile_IFStream(const char* path, int usehunk, uintptr_t* path_id)
{
	cxxifstream         f = {};
    T* buf				= nullptr;
    char base[32]		= {};
    size_t	len			= 0;

    // look for it in the filesystem or pack files
    len = g_Common->COM_FOpenFile_IFStream(path, &f, path_id);
    if (f.bad())
        return nullptr;

    // extract the filename base name for hunk tag
    g_Common->COM_FileBase(path, base, len);

    switch (usehunk)
    {
    case HUNK_FULL:
        buf = static_cast<T*>(g_MemCache->Hunk_AllocName<T>(len + 1, base));
        break;
    case HUNK_TEMP:
        buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
        break;
    case HUNK_ZONE:
        buf = static_cast<T*>(mainzone->Z_Malloc<T>(len + 1));
        break;
    case HUNK_CACHE:
        buf = static_cast<T*>(g_MemCache->Cache_Alloc<T>(loadcache, len + 1, base));
        break;
    case HUNK_TEMP_FILL:
        if (len + 1 > loadsize<T>)
        {
            buf = static_cast<T*>(g_MemCache->Hunk_TempAlloc<T>(len + 1));
            break;
        }
        else
        {
            buf = loadbuf<T>;
            break;
        }

    default:

        Sys_Error("COM_LoadFile: bad usehunk");
        break;

    }

    if (!buf)
        Sys_Error("COM_LoadFile: not enough space for %s", path);
    else
        ((byte*)buf)[len] = 0;

    f.read(buf, len);

    return buf;
}

template<typename T>
T* COM_LoadHunkFile(const char* path, uintptr_t* path_id)
{
	return COM_LoadFile<T>(path, HUNK_FULL, path_id);
}

template<typename T>
T* COM_LoadHunkFile_IFStream(const char* path, uintptr_t* path_id)
{
    return COM_LoadFile_IFStream<T>(path, HUNK_FULL, path_id);
}

template<typename T>
T* COM_LoadTempFile(const char* path, uintptr_t* path_id)
{
	return COM_LoadFile<T>(path, HUNK_TEMP, path_id);
}

template<typename T>
void COM_LoadCacheFile(const char* path, struct cache_user_s* cu, uintptr_t* path_id)
{
	loadcache = cu;
	COM_LoadFile<T>(path, HUNK_CACHE, path_id);
}

// uses temp hunk if larger than bufsize
template<typename T>
T* COM_LoadStackFile(const char* path, void* buffer, int bufsize, uintptr_t* path_id)
{
	T* buf;

	loadbuf<T> = (T*)buffer;
	loadsize<T> = bufsize;
	buf = COM_LoadFile<T>(path, HUNK_TEMP_FILL, path_id);

	return (T*)buf;
}

template<typename T>
T* COM_LoadStackFile_VPK(const char* path, void* buffer, int bufsize, uintptr_t* path_id)
{
	T* buf;

	loadbuf<T> = (T*)buffer;
	loadsize<T> = bufsize;

	buf = COM_LoadFile_VPK<T>(path, HUNK_TEMP_FILL, path_id);

	if (!buf)
		return nullptr;

	return (T*)buf;
}


#endif
