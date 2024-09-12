/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schmiedeberg

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

void MSG_BeginReading ();
int MSG_ReadChar ();
int MSG_ReadByte ();
int MSG_ReadShort ();
int MSG_ReadLong ();
float MSG_ReadFloat ();
const char *MSG_ReadString ();

float MSG_ReadCoord ();
float MSG_ReadAngle ();

//============================================================================

void Q_memset (void *dest, int fill, size_t count);
void Q_memcpy (void *dest, const void *src, size_t count);
int Q_memcmp (void *m1, void *m2, size_t count);
void Q_strcpy (char *dest, const char *src);
void Q_strncpy (char *dest, const char *src, size_t count);
int Q_strlen (const char *str);

char* Q_strlwr(char* s1);
char* Q_strupr(char* s1);

char *Q_strrchr (const char *s, char c);
void Q_strcat (char *dest, const char *src);
int Q_strcmp (const char *s1, const char *s2);
int Q_strncmp (const char *s1, const char *s2, size_t count);
int Q_strcasecmp (const char *s1, const char *s2);
int Q_strncasecmp (const char *s1, const char *s2, size_t n);
int	Q_atoi (const char *str);
float Q_atof (const char *str);
int Q_vsnprintf_s(char* str, size_t size, size_t len, const char* format, va_list args);

int Q_snscanf(const char* buffer, size_t bufsize, const char* fmt, ...);

void Q_FixSlashes(char* str, size_t size, const char delimiter = '\\');
void Q_FixQuotes(char* dest, const char* src, size_t size);

int Q_stricmp(const char* s1, const char* s2);
int Q_stricmpn(const char* s1, const char* s2, int n);

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
	void COM_CheckRegistered();
	void COM_InitArgv (int argc, char **argv);

	void COM_AddExtension(char* path, const char* extension, size_t len);	// Missi: copied from QuakeSpasm (1/10/2023)

	char* COM_SkipPath (const char *pathname);
	void COM_StripExtension (const char *in, char *out);
	const char* COM_FileExtension(const char* in);
	void COM_FileBase(const char* in, char* out, size_t outsize);
	void COM_DefaultExtension (char *path, const char *extension);
	const char* COM_FileGetExtension(const char* in);
	bool COM_FileExists(const char* filename, uintptr_t* path_id);
	static void COM_CopyFile(const char* netpath, char* cachepath);
	void COM_WriteFile (const char *filename, void *data, int len);

	static void COM_CreatePath(const char* path);
	int COM_OpenFile (const char *filename, int *handle, uintptr_t* path_id);
	static int COM_FOpenFile (const char *filename, FILE **file, uintptr_t* path_id);
    int COM_FOpenFile_IFStream(const char* filename, cxxifstream* file, uintptr_t* path_id, unzFile* pk3_file = nullptr);
    void COM_FOpenFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id);
	cxxifstream* COM_FOpenFile_VPK(const char* filename, uintptr_t* path_id);
	void COM_CloseFile (int h);

	byte* COM_LoadMallocFile_TextMode_OSPath(const char* path, long* len_out);

	void COM_AddGameDirectory(const char* dir);

	void COM_InitFilesystem();
	static int COM_FindFile(const char* filename, int* handle, FILE** file, uintptr_t* path_id);
    int COM_FindFile_IFStream(const char* filename, int* handle, cxxifstream* file, uintptr_t* path_id, unzFile* pk3_file);
    void COM_FindFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id);
    cxxifstream* COM_FindFile_VPK(const char* filename, uintptr_t* path_id);
	static long COM_filelength(FILE* f);
    size_t COM_filelength_FStream(cxxifstream* f);
	pack_t* COM_LoadPackFile(char* packfile);

	static void COM_Path_f();

	// Missi: buffer-safe varargs (4/30/2023)
	const char	*va(const char *format, ...);

    static	char	com_token[1024];
	static	bool	com_eof;

	static	int		com_argc;
	static	char	**com_argv;

	static	int		com_filesize;
	static	char	com_gamedir[MAX_OSPATH];
	static	char	com_cachedir[MAX_OSPATH];
	static	int		file_from_pak;
	static	int		file_from_pk3;
	static	int		file_from_vpk;
	static	unzFile	current_pk3;

};

unsigned	Com_BlockChecksum(const void* buffer, int length);
unsigned	Com_BlockChecksumKey(void* buffer, int length, int key);

#define MAX_ZPATH			256
#define	MAX_SEARCH_PATHS	4096
#define MAX_FILEHASH_SIZE	1024

typedef union qfile_gus {
	FILE* o;
	unzFile		z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut	file;
	bool		unique;
} qfile_ut;

typedef struct {
	qfile_ut	handleFiles;
	bool		handleSync;
	int			baseOffset;
	int			fileSize;
	int			zipFilePos;
	bool		zipFile;
	bool		streamed;
	char		name[MAX_ZPATH];
} fileHandleData_t;

extern fileHandleData_t	fsh[MAX_FILE_HANDLES];
extern fileHandleData_t	fsh_ifstream[MAX_FILE_HANDLES];

extern CCommon* g_Common;

//============================================================================
// Missi: PK3 support (8/28/2024)

typedef struct fileInPack_s {
	char* name;		// name of the file
	unsigned long			pos;		// file info position in zip
	struct	fileInPack_s* next;		// next file in the hash
} fileInPack_t;

typedef struct {
	char			pakFilename[MAX_OSPATH];	// c:\quake3\baseq3\pak0.pk3
	char			pakBasename[MAX_OSPATH];	// pak0
	char			pakGamename[MAX_OSPATH];	// baseq3
	unzFile			handle;						// handle to zip file
	int				checksum;					// regular checksum
	int				pure_checksum;				// checksum for pure
	int				numfiles;					// number of files in pk3
	int				referenced;					// referenced file flags
	int				hashSize;					// hash table size (power of 2)
	fileInPack_t** hashTable;					// hash table
	fileInPack_t* buildBuffer;					// buffer with the filenames etc.
} pack_pk3_t;

typedef struct {
	char		path[MAX_OSPATH];		// c:\quake3
	char		gamedir[MAX_OSPATH];	// baseq3
} directory_t;

//============================================================================
// Missi: Repurposed QuakeSpasm filesystem stuff with Quake III
// additions (1/1/2023)

#define	FS_ENT_NONE			(0)
#define	FS_ENT_FILE			(1 << 0)
#define	FS_ENT_DIRECTORY	(1 << 1)

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

	pack_pk3_t* FS_LoadZipFile(const char* zipfile, const char* basename);
	long FS_HashFileName(const char* fname, int hashSize);

	int FS_HandleForFile();

private:
#if (__linux__) || (__apple__)
	static constexpr char PATH_SEP = '/';
#else
	static constexpr char PATH_SEP = '\\';
#endif
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
	{
		if (g_Common->file_from_pk3)
		{
			g_Common->file_from_pk3 = 0;
			unzReadCurrentFile(fsh[h].handleFiles.file.z, buf, len);

			unzCloseCurrentFile(fsh[h].handleFiles.file.z);

			g_Common->COM_CloseFile(h);

			return buf;
		}
	}

#if 0
#ifndef GLQUAKE
	g_SoftwareRenderer->Draw_BeginDisc();
#else
	g_GLRenderer->Draw_BeginDisc();
#endif
#endif

	Sys_FileRead(h, buf, len);
	g_Common->COM_CloseFile(h);

	((byte*)buf)[len] = 0;

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
	unzFile	unzf = {};

    // look for it in the filesystem or pack files
    len = g_Common->COM_FOpenFile_IFStream(path, &f, path_id, &unzf);
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
	{
		if (g_Common->file_from_pk3)
		{
			g_Common->file_from_pk3 = 0;

			unzReadCurrentFile(unzf, buf, len);

			f.close();
			unzCloseCurrentFile(unzf);
			return buf;
		}
	}
        ((byte*)buf)[len] = 0;

    f.read(buf, len);
	f.close();

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
