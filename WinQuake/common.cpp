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
// common.c -- misc functions used in client and server

#include "quakedef.h"

#include <errno.h>

// Missi: FIXME: disable the MSVC "unsafe" function warning for now, but eventually move ALL file-handling code to ifstream,
// as fopen is unsafe (9/10/2024)
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

CCommon* g_Common;
CFileSystem* g_FileSystem;
cache_user_s* loadcache;

constexpr int NUM_SAFE_ARGVS = 7;

static char*			largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static const char*		argvdummy;

static const char*     safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"};


cvar_t  registered = {"registered","0"};
static cvar_t  cmdline = {"cmdline","0", false, true};

static bool	com_modified;   // set true if using non-id files
static bool	proghack;
static int	static_registered = 1;  // only for startup check, then set
static bool	msg_suppress_1 = 0;

// if a packfile directory differs from this, it is assumed to be hacked
static constexpr int PAK0_COUNT	= 339;
static constexpr int PAK0_CRC	= 32981;

char	CCommon::com_token[1024];
int		CCommon::com_argc = 0;
char**	CCommon::com_argv = nullptr;
bool	CCommon::com_eof = false;
int		CCommon::com_filesize = 0;
char    CCommon::com_cachedir[MAX_OSPATH];
char    CCommon::com_gamedir[MAX_OSPATH];
int		CCommon::file_from_pak = 0;
int		CCommon::file_from_pk3 = 0;
int		CCommon::file_from_vpk = 0;
unzFile	CCommon::current_pk3 = nullptr;

constexpr int CMDLINE_LENGTH = 256;
static char	com_cmdline[CMDLINE_LENGTH];

bool		standard_quake = true, rogue, hipnotic;

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.
	
*/

//============================================================================
// 
// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Q_memset (void *dest, int fill, size_t count)
{
	int             i;

	if ( (((VOID_P)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i=0 ; i<count ; i++)
			((byte *)dest)[i] = fill;
}

void Q_memcpy (void *dest, const void *src, size_t count)
{
	int             i;

	if ((((VOID_P)dest | (VOID_P)src | count) & 3) == 0 )
	{
		count>>=2;
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i=0 ; i<count ; i++)
			((byte *)dest)[i] = ((byte *)src)[i];
}

int Q_memcmp (void *m1, void *m2, size_t count)
{
	while(count)
	{
		count--;
		if (((byte *)m1)[count] != ((byte *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy (char *dest, const char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, const char *src, size_t count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen (const char *str)
{
	int	count = 0;

	while (str[count])
		count++;

	return count;
}

char* Q_strlwr(char* s1) {
	char* s;

	s = s1;
	while (*s) {
		*s = tolower(*s);
		s++;
	}
	return s1;
}

char* Q_strupr(char* s1) {
	char* s;

	s = s1;
	while (*s) {
		*s = toupper(*s);
		s++;
	}
	return s1;
}

char *Q_strrchr(const char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
	if (*--s == c) return (char*)s;
	return NULL;
}

void Q_strcat (char *dest, const char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp (const char *s1, const char*s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int Q_strncmp (const char*s1, const char*s2, size_t count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int Q_strncasecmp (const char *s1, const char *s2, size_t n)
{
	int             c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}
	
	return -1;
}

int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

int Q_atoi (const char *str)
{
	int             val;
	int             sign;
	int             c;
	
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}
	
	return 0;
}


float Q_atof (const char *str)
{
	double			val;
	int             sign;
	int             c;
	int             decimal, total;
	
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}
	
	return val*sign;
}

/*
==================
Missi: Q_snscanf
Buffer-safe version of sscanf that is portable, so programmers
won't need to use the non-portable sscanf_s (9/8/2024)
==================
*/
int Q_snscanf(const char* buffer, size_t bufsize, const char* fmt, ...)
{
	int fmt_pos = 0;			// Missi: position in format string (9/8/2024)
	int buffer_pos = 0;			// Missi: position in buffer (9/9/2024)

	va_list args = {};
	va_start(args, fmt);
	bool length = false;	// Missi: defines whether a length was specified in the format string (9/8/2024)
	int charcount = 0;		// Missi: independent of fmt_pos and also counts number of chars read (9/8/2024)
	int filled = 0;
	
	while (buffer[buffer_pos] != '\0' && buffer_pos < bufsize)
	{
		if (fmt[fmt_pos] == '\0')
			break;

		if (fmt[fmt_pos] == '%')
		{
			if (fmt[fmt_pos + 1] >= '0' && fmt[fmt_pos + 1] <= '9')
			{
				int num = 0;
				char jump[64];
				memset(jump, 0, sizeof(jump));

				num = Q_atoi(&fmt[fmt_pos + 1]);
				snprintf(jump, sizeof(jump), "%d", num);

				fmt_pos += Q_strlen(jump) + 1;
				length = true;
			}

			if (!length)
				fmt_pos++;

			switch (fmt[fmt_pos])
			{
				case '\r':
				case '\n':
					fmt_pos++;
					break;

				// Missi: standard int (9/8/2024)
				case 'd':
				case 'i':
				{
					while (buffer[charcount++] != '\n' && buffer[charcount] != '\0')
						;

					int copy = Q_atoi(&buffer[buffer_pos]);
					memcpy(va_arg(args, int*), &copy, sizeof(int));
					filled++;
					break;
				}
				// Missi: standard float (9/8/2024)
				case 'f':
				{
					while (buffer[charcount++] != '\n' && buffer[charcount] != '\0')
						;

					float copy = Q_atof(&buffer[buffer_pos]);
					memcpy(va_arg(args, float*), &copy, sizeof(float));
					filled++;
					break;
				}
				// Missi: current number of chars read (9/9/2024)
				case 'n':
				{
					memcpy(va_arg(args, int*), &charcount, sizeof(int));
					filled++;
					break;
				}
				// Missi: const char* (9/9/2024)
				case 's':
				{
					while (buffer[charcount++] != '\n' && buffer[charcount] != '\0')
						;

					va_list va_arg_copy;
					va_copy(va_arg_copy, args);

					memcpy(va_arg(args, char*), &buffer[0], charcount - 1);
					va_arg(va_arg_copy, char*)[charcount - 1] = '\0';
					filled++;
					break;
				}
				// Missi: unsigned integers (9/9/2024)
				case 'u':
				{

					if (fmt[fmt_pos + 1] == 'l' && fmt[fmt_pos + 2] == 'l')
					{
						unsigned long long copy = atoll(&buffer[buffer_pos]);
						memcpy(va_arg(args, unsigned long long*), &copy, sizeof(unsigned long long));
						filled++;
					}
					else if (fmt[fmt_pos + 1] == 'l')
					{
						unsigned long copy = atol(&buffer[buffer_pos]);
						memcpy(va_arg(args, unsigned long*), &copy, sizeof(unsigned long));
						filled++;
					}
					else
					{
						unsigned int copy = Q_atoi(&buffer[buffer_pos]);
						memcpy(va_arg(args, unsigned int*), &copy, sizeof(unsigned int));
						filled++;
					}
					break;
				}
				// Missi: hexadecimal output (9/9/2024)
				case 'x':
				{
					unsigned int copy = Q_atoi(&buffer[buffer_pos]);

					std::stringstream hex_strstream;

					hex_strstream << '0' << 'x' << std::hex << copy;

					cxxstring hex_output = hex_strstream.str();
					const char* hex_output_c = hex_output.c_str();

					Q_strncpy(va_arg(args, char*), hex_output_c, bufsize);
					filled++;
					break;
				}
				// Missi: single char (9/9/2024)
				case 'c':
				{
					memcpy(va_arg(args, char*), &buffer[buffer_pos], sizeof(char));
					filled++;
					break;
				}
			}
		}

		length = false;
		fmt_pos++;
	}
	va_end(args);

	return filled;
}

// Missi: copied from Quakespasm (5/22/22)

/* platform dependant (v)snprintf function names: */
#if defined(_WIN32) && defined(WIN64)
#define	snprintf_func		_snprintf_s
#define	vsnprintf_func		_vsnprintf
#define	vsnprintf_s_func	_vsnprintf_s
#else
#define	snprintf_func		snprintf
#define	vsnprintf_func		vsnprintf
// #define	vsnprintf_s_func	vsnprintf_s
#endif

int Q_vsnprintf_s(char* str, size_t size, size_t len, const char* format, va_list args)
{
	int		ret;
#ifndef WIN64
	ret = snprintf_func(str, len, format, args);
#elif WIN64
	ret = vsnprintf_s_func(str, size, len, format, args);
#else
	ret = snprintf_func(str, len, format, args);    
#endif
	if (ret < 0)
		ret = (int)size;
	if (size == 0)	/* no buffer */
		return ret;
	if ((size_t)ret >= size)
		str[size - 1] = '\0';

	return ret;
}

void Q_FixSlashes(char* str, size_t size, const char delimiter)
{
	if (!size || !str)
		return;

	for (int pos = 0; str[pos]; pos++)
	{
		if (str[pos] == delimiter)
		{
			str[pos] = '/';
		}
	}
}

void Q_FixQuotes(char* dest, const char* src, size_t size)
{
	if (!size || !src)
		return;

	while(*src && --size)
	{
		if (!size)
			break;

		if (*src != '\"')
			*dest = *src;
		else
			*dest = *++src;

		src++; dest++;
	}
	*dest = '\0';
}

int Q_stricmp(const char* s1, const char* s2) {
	return (s1 && s2) ? Q_stricmpn(s1, s2, 99999) : -1;
}

int Q_stricmpn(const char* s1, const char* s2, int n) {
	int		c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
	if (s1 == NULL) {
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
		return 1;



	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

bool        bigendien;

short   (*BigShort) (short l);
short   (*LittleShort) (short l);
int     (*BigLong) (int l);
int     (*LittleLong) (int l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int     LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		byte    b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = static_cast<byte*>(SZ_GetSpace (sb, 1));
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte    *buf = NULL;
	
#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (byte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (byte*)SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte    *buf;
	
	buf = (byte*)SZ_GetSpace(sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float   f;
		int     l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, (char*)s, Q_strlen(s)+1);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, ((int)f*256/360) & 255);
}

//
// reading functions
//
int                     msg_readcount;
bool        msg_badread;

void MSG_BeginReading ()
{
	msg_readcount = 0;
	msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadByte ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadShort ()
{
	int     c;
	
	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8));
	
	msg_readcount += 2;
	
	return c;
}

int MSG_ReadLong ()
{
	int     c;
	
	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8)
	+ (net_message.data[msg_readcount+2]<<16)
	+ (net_message.data[msg_readcount+3]<<24);
	
	msg_readcount += 4;
	
	return c;
}

float MSG_ReadFloat ()
{
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	
	dat.b[0] =      net_message.data[msg_readcount];
	dat.b[1] =      net_message.data[msg_readcount+1];
	dat.b[2] =      net_message.data[msg_readcount+2];
	dat.b[3] =      net_message.data[msg_readcount+3];
	msg_readcount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;   
}

const char *MSG_ReadString ()
{
	static char     string[2048];
	int             l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

float MSG_ReadCoord ()
{
	return MSG_ReadFloat();
}

float MSG_ReadAngle ()
{
	return MSG_ReadChar() * (360.0/256);
}



//===========================================================================

void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = g_MemCache->Hunk_AllocName<byte>(startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}


void SZ_Free (sizebuf_t *buf)
{
//      Z_Free (buf->data);
//      buf->data = NULL;
//      buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void    *data = 0;
	
	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Sys_Error ("SZ_GetSpace: overflow without allowoverflow set");
		
		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);
			
		buf->overflowed = true;
		Con_Printf ("SZ_GetSpace: overflow");
		SZ_Clear (buf); 
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);         
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int             len;
	
	len = Q_strlen(data)+1;

// byte * cast to keep VC++ happy
	if (buf->data[buf->cursize-1])
		Q_memcpy ((byte *)SZ_GetSpace(buf, len),(void*)data,len); // no trailing 0
	else
		Q_memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,(void*)data,len); // write over trailing 0
}


//============================================================================

/*
==================
COM_AddExtension
if path extension doesn't match .EXT, append it
(extension should include the leading ".")
==================
*/
void CCommon::COM_AddExtension(char* path, const char* extension, size_t len)
{
	if (strcmp(COM_FileGetExtension(path), extension + 1) != 0)
		Q_strlcat(path, extension, len);
}

/*
============
COM_SkipPath

Missi: modified (4/30/2023)
============
*/
char* CCommon::COM_SkipPath (const char *pathname)
{
	char    *last;
	
	last = _strdup(pathname);
	while (*pathname)
	{
		if (*pathname=='/')
			last = _strdup(pathname+1);
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void CCommon::COM_StripExtension (const char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FileExtension
============
*/
const char* CCommon::COM_FileExtension (const char *in)
{
	static char exten[8];
	int             i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
Missi: copied from QuakeSpasm. Original function was very haphazard
============
*/
void CCommon::COM_FileBase(const char* in, char* out, size_t outsize)
{
	const char* dot, * slash, * s;

	s = in;
	slash = in;
	dot = NULL;
	while (*s)
	{
		if (*s == '/')
			slash = s + 1;
		if (*s == '.')
			dot = s;
		s++;
	}
	if (dot == NULL)
		dot = s;

	if (dot - slash < 2)
		Q_strlcpy(out, "?model?", outsize);
	else
	{
		size_t	len = dot - slash;
		if (len >= outsize)
			len = outsize - 1;
		memcpy(out, slash, len);
		out[len] = '\0';
	}
}


/*
==================
COM_DefaultExtension
==================
*/
void CCommon::COM_DefaultExtension (char *path, const char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	Q_strcat (path, extension);
}

/*
==================
Missi: COM_FileGetExtension (1/1/2023)
==================
*/
const char* CCommon::COM_FileGetExtension(const char* in)
{
	const char* src;
	size_t		len;

	len = strlen(in);
	if (len < 2)	/* nothing meaningful */
		return "";

	src = in + len - 1;
	while (src != in && src[-1] != '.')
		src--;
	if (src == in || strchr(src, '/') != NULL || strchr(src, '\\') != NULL)
		return "";	/* no extension, or parent directory has a dot */

	return src;
}

/*
===========
COM_FileExists

Returns whether the file is found in the quake filesystem.
===========
*/
bool CCommon::COM_FileExists(const char* filename, uintptr_t* path_id)
{
	int ret = COM_FindFile(filename, NULL, NULL, path_id);
	return (ret == -1) ? false : true;
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char* CCommon::COM_Parse (const char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
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
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
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

const char* CCommon::COM_ParseIntNewline(const char* buffer, int* value)
{
	int consumed = 0;
	Q_snscanf(buffer, 1024, "%i\n%n", value, &consumed);
	return buffer + consumed;
}

const char* CCommon::COM_ParseFloatNewline(const char* buffer, float* value)
{
	int consumed = 0;
	Q_snscanf(buffer, 1024, "%f\n%n", value, &consumed);
	return buffer + consumed;
}

const char* CCommon::COM_ParseStringNewline(const char* buffer)
{
	int consumed = 0;
	com_token[0] = '\0';
	Q_snscanf(buffer, 1024, "%1023s\n%n", com_token, &consumed);
	return buffer + consumed;
}

const int CCommon::COM_ParseStringLength(const char* buffer, size_t len) const
{
	int pos = 0;

	if (buffer[pos] != '\"')
		return 0;

	pos++;

	while ((buffer[pos]) && (pos < len) && (buffer[pos]) != '\"')
		pos++;

	return pos;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int CCommon::COM_CheckParm (const char *parm)
{
	int             i;
	
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp (parm,com_argv[i]))
			return i;
	}
		
	return 0;
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void CCommon::COM_CheckRegistered ()
{
	unsigned short  check[128];
	int                     i;
	cxxifstream		f;

	COM_FOpenFile_IFStream("gfx/pop.lmp", &f, nullptr);
	static_registered = 0;

	if (!f.is_open())
	{
#if WINDED
	Sys_Error ("This dedicated server requires a full registered copy of Quake");
#endif
		Con_Printf ("Playing shareware version.\n");
		if (com_modified)
			Sys_Error ("You must have the registered version to use modified games");
		return;
	}

	f.read((char*)check, sizeof(check));
	f.close();
	
	for (i=0 ; i<128 ; i++)
		if (pop[i] != (unsigned short)BigShort (check[i]))
			Sys_Error ("Corrupted data file.");
	
	Cvar_Set ("cmdline", com_cmdline);
	Cvar_Set ("registered", "1");
	static_registered = 1;
	Con_Printf ("Playing registered version.\n");
}


/*
================
COM_InitArgv
================
*/
void CCommon::COM_InitArgv (int argc, char **argv)
{
	bool        safe;
	int             i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j=0 ; (j<MAX_NUM_ARGVS) && (j< argc) ; j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	com_cmdline[n] = 0;

	safe = false;

	for (com_argc=0 ; (com_argc<MAX_NUM_ARGVS) && (com_argc < argc) ;
		 com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp ("-safe", argv[com_argc]))
			safe = true;
	}

	if (safe)
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i=0 ; i<NUM_SAFE_ARGVS ; i++)
		{
			Q_strlcpy(largv[com_argc], safeargvs[i], strlen(safeargvs[i]));
			// largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	com_argv = largv;

	if (COM_CheckParm ("-rogue"))
	{
		rogue = true;
		standard_quake = false;
	}

	if (COM_CheckParm ("-hipnotic"))
	{
		hipnotic = true;
		standard_quake = false;
	}
}


/*
================
COM_Init
================
*/
void CCommon::COM_Init (const char *basedir)
{
	byte    swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner 
	if ( *(short *)swaptest == 1)
	{
		bigendien = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	Cvar_RegisterVariable (&registered);
	Cvar_RegisterVariable (&cmdline);
	g_pCmds->Cmd_AddCommand ("path", CCommon::COM_Path_f);

	COM_InitFilesystem ();
	COM_CheckRegistered ();
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
Missi: made this buffer-size safe (4/30/2023)
============
*/
const char* CCommon::va(const char *format, ...)
{
	va_list			argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	return string;  
}

/// just for debugging
int     memsearch (byte *start, int count, int search)
{
	int             i;
	
	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/
/*
=============================================================================
Missi: There are multiple ways to handle files in BananaQuake.

* Using COM_FOpenFile for standard files within PAK files or game directory 
files, that uses C's FILE struct. This tends to work for reading files, not
so much for writing files that are already opened.

* Using COM_FOpenFile_IFStream for reading, which takes advantage of C++'s 
ifstream class. This tends to work better and play nicer with operating 
systems that are strict about file's being opened when reading.

* Using COM_FOpenFile_OFStream for writing, which takes advantage of C++'s
ofstream class. This tends to work better and play nicer with operating
systems that are strict about file's being opened when writing.

* Using COM_FOpenFile_VPK for reading files out of Valve package files.
This tends to work better and play nicer with operating systems that
are strict about file's being opened when writing.
=============================================================================
*/

static int     com_filesize;

static constexpr unsigned int MAX_FILES_IN_PACK = 8192;    // Missi: was 2048 (7/12/2024)

struct searchpath_t
{
	uintptr_t path_id;
	char    filename[MAX_OSPATH] = "";
	pack_t  *pack = NULL;          // only one of filename / pack will be used
	pack_pk3_t	*pk3 = NULL;
	struct searchpath_t *next = NULL;
};

searchpath_t* com_searchpaths = nullptr;

static	char		fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
static	cvar_t*		fs_debug;
static	cvar_t*		fs_homepath;
static	cvar_t*		fs_basepath;
static	cvar_t*		fs_basegame;
static	cvar_t*		fs_cdpath;
static	cvar_t*		fs_copyfiles;
static	cvar_t*		fs_gamedirvar;
static	cvar_t*		fs_restrict;
static	searchpath_t* fs_searchpaths;
static	int			fs_readCount;			// total bytes read
static	int			fs_loadCount;			// total files read
static	int			fs_loadStack;			// total files in memory
static	int			fs_packFiles;			// total number of files in packs

static int			fs_fakeChkSum;
static int			fs_checksumFeed;

fileHandleData_t	fsh[MAX_FILE_HANDLES];
fileHandleData_t	fsh_ifstream[MAX_FILE_HANDLES];

/*
============
COM_Path_f

============
*/
void CCommon::COM_Path_f ()
{
	searchpath_t    *s;
	
	Con_Printf ("Current search path:\n");
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
		{
			Con_Printf ("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf ("%s\n", s->filename);
	}
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void CCommon::COM_WriteFile (const char *filename, void *data, int len)
{
	int		handle;
	char	name[MAX_OSPATH];
	
	snprintf (name, sizeof(name), "%s/%s", com_gamedir, filename);

	handle = Sys_FileOpenWrite (name);
	if (handle == -1)
	{
		Sys_Printf ("COM_WriteFile: failed on %s\n", name);
		return;
	}
	
	Sys_Printf ("COM_WriteFile: %s\n", name);
	Sys_FileWrite (handle, data, len);
	Sys_FileClose (handle);
}


/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void CCommon::COM_CreatePath (const char *path)
{
	char    *ofs;

	ofs = _strdup(path);

	for (; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{       // create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
void CCommon::COM_CopyFile (const char *netpath, char *cachepath)
{
	int             in, out;
	int             remaining, count;
	char    buf[4096];
	
	remaining = Sys_FileOpenRead (netpath, &in);            
	COM_CreatePath (cachepath);     // create directories up to the cache file
	out = Sys_FileOpenWrite (cachepath);
	
	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		Sys_FileRead (in, buf, count);
		Sys_FileWrite (out, buf, count);
		remaining -= count;
	}

	Sys_FileClose (in);
	Sys_FileClose (out);    
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
int CCommon::COM_FindFile (const char *filename, int *handle, FILE **file, uintptr_t* path_id)
{
	searchpath_t	*search = nullptr;
	char			netpath[MAX_OSPATH];
	pack_t			*pak = nullptr;
	pack_pk3_t		*pk3 = nullptr;
	int				i = 0;
	int				findtime = 0, cachetime = 0;

	fileInPack_t*	pakFile;
	unz_s*	zfi;
	cxxifstream*	temp;

	if (file && handle)
		Sys_Error ("COM_FindFile: both handle and file set");
	
	file_from_pak = 0;
	file_from_pk3 = 0;

//
// search through the path, one element at a time
//
	for (search = com_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)	/* look through all the pak file elements */
		{
			pak = search->pack;
			for (i = 0; i < pak->numfiles; i++)
			{
				if (strcmp(pak->files[i].name, filename) != 0)
					continue;
				// found it!
				com_filesize = pak->files[i].filelen;
				file_from_pak = 1;
				if (path_id)
					*path_id = search->path_id;
				if (handle)
				{
					*handle = pak->handle;
					Sys_FileSeek (pak->handle, pak->files[i].filepos);
					return com_filesize;
				}
				else if (file)
				{ /* open a new file on the pakfile */
					*file = fopen (pak->filename, "rb");
					if (*file)
						fseek (*file, pak->files[i].filepos, SEEK_SET);
					return com_filesize;
				}
				else /* for COM_FileExists() */
				{
					return com_filesize;
				}
			}
		}
		else if (search->pk3)
		{
			pk3 = search->pk3;

			long hash = g_FileSystem->FS_HashFileName(filename, pk3->hashSize);

			if (search->pk3->hashTable[hash])
			{
				pakFile = pk3->hashTable[hash];

				do
				{
					if (!strcmp(pakFile->name, filename))
					{

						// found it!
						// com_filesize = pk3->buildBuffer[i].filelen;
						file_from_pk3 = 1;
						if (path_id)
							*path_id = search->path_id;
						if (handle)
						{
							Sys_FileOpenRead(pk3->pakFilename, handle);

							fsh[*handle].handleFiles.file.z = unzReOpen(pk3->pakFilename, pk3->handle);
							if (fsh[*handle].handleFiles.file.z == NULL)
							{
								Sys_Error("Couldn't reopen %s", pk3->pakFilename);
							}
							else
							{
								fsh[*handle].handleFiles.file.z = pk3->handle;
							}

							Q_strncpy(fsh[*handle].name, filename, sizeof(fsh[*handle].name));
							fsh[*handle].zipFile = true;
							zfi = (unz_s*)fsh[*handle].handleFiles.file.z;
							// in case the file was new
							temp = zfi->ifstr;
							// set the file position in the zip file (also sets the current file info)
							unzSetCurrentFileInfoPosition(pk3->handle, pakFile->pos);
							// copy the file info into the unzip structure
							memcpy(zfi, pk3->handle, sizeof(unz_s));
							// we copy this back into the structure
							zfi->ifstr = temp;
							// open the file in the zip
							unzOpenCurrentFile(fsh[*handle].handleFiles.file.z);
							fsh[*handle].zipFilePos = pakFile->pos;

							com_filesize = zfi->cur_file_info.uncompressed_size;

							return zfi->cur_file_info.uncompressed_size;
						}
						else if (file)
						{ /* open a new file on the pakfile */

							*file = fopen(pk3->pakFilename, "rb");
							if (*file)
								fseek(*file, pakFile->pos, SEEK_SET);

							int hndl = g_FileSystem->FS_HandleForFile();

							fsh[hndl].handleFiles.file.z = file;
							sys_handles[hndl] = *file;

							Q_strncpy(fsh[hndl].name, filename, sizeof(fsh[hndl].name));
							fsh[hndl].zipFile = true;
							zfi = (unz_s*)fsh[hndl].handleFiles.file.z;
							// in case the file was new
							temp = zfi->ifstr;
							// set the file position in the zip file (also sets the current file info)
							unzSetCurrentFileInfoPosition(pk3->handle, pakFile->pos);
							// copy the file info into the unzip structure
							memcpy(zfi, pk3->handle, sizeof(unz_s));
							// we copy this back into the structure
							zfi->ifstr = temp;
							// open the file in the zip
							unzOpenCurrentFile(fsh[hndl].handleFiles.file.z);
							fsh[hndl].zipFilePos = pakFile->pos;

							com_filesize = zfi->cur_file_info.uncompressed_size;
							
							return com_filesize;
						}
						else /* for COM_FileExists() */
						{
							return com_filesize;
						}
					}

					pakFile = pakFile->next;
				} while (pakFile != nullptr);
			}
		}
		else
		{
			if (!registered.value)
			{ /* if not a registered version, don't ever go beyond base */
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}

			snprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			if (! (Sys_FileType(netpath) & FS_ENT_FILE))
				continue;

			if (path_id)
				*path_id = search->path_id;
			if (handle)
			{
				com_filesize = Sys_FileOpenRead (netpath, &i);
				*handle = i;
				return com_filesize;
			}
			else if (file)
			{
				*file = fopen (netpath, "rb");
				com_filesize = (*file == NULL) ? -1 : COM_filelength (*file);
				return com_filesize;
			}
			else
			{
				return 0; /* dummy valid value for COM_FileExists() */
			}
		}
	}
	
	Sys_Printf ("FindFile: can't find %s\n", filename);
	
	if (handle)
		*handle = -1;
	if (file)
		*file = NULL;
	com_filesize = -1;
	return com_filesize;
}
/*
===========
Missi: COM_FindFile_IFStream

Finds the file in the search path.
Sets com_filesize and one of handle or std::ifstream.

FIXME: This function does not work with PK3 files due to the zip library making
heavy usage of the FILE struct from C. That is lame. Write a derivative later to take advantage
of std::ifstream so PK3 files can be used. (8/29/2024)
===========
*/
int CCommon::COM_FindFile_IFStream(const char* filename, int* handle, cxxifstream* file, uintptr_t* path_id, unzFile* pk3_file)
{
	searchpath_t*	search = NULL;
	char			netpath[MAX_OSPATH];
	pack_t*			pak = NULL;
	pack_pk3_t*		pk3 = NULL;
	int				i = 0;
	int				findtime = 0, cachetime = 0;

	fileInPack_t* pakFile;
	unz_s* zfi;
	cxxifstream* temp;

	if (file && handle)
		Sys_Error("COM_FindFile: both handle and file set");

	file_from_pak = 0;
	file_from_pk3 = 0;

	//
	// search through the path, one element at a time
	//
	for (search = com_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)	/* look through all the pak file elements */
		{
			pak = search->pack;
			for (i = 0; i < pak->numfiles; i++)
			{
				if (strcmp(pak->files[i].name, filename) != 0)
					continue;
				// found it!
				com_filesize = pak->files[i].filelen;
				file_from_pak = 1;
				if (path_id)
					*path_id = search->path_id;
				if (handle)
				{
					*handle = pak->handle;
					Sys_FileSeek(pak->handle, pak->files[i].filepos);
					return com_filesize;
				}
				else if (file)
				{ /* open a new file on the pakfile */
					file->open(pak->filename, cxxifstream::binary | cxxifstream::in);
					file->seekg(pak->files[i].filepos, cxxifstream::beg);
					return com_filesize;
				}
				else /* for COM_FileExists() */
				{
					return com_filesize;
				}
			}
		}
		else if (search->pk3)
		{
			pk3 = search->pk3;

			long hash = g_FileSystem->FS_HashFileName(filename, pk3->hashSize);

			if (search->pk3->hashTable[hash])
			{
				pakFile = pk3->hashTable[hash];

				do
				{
					if (!strcmp(pakFile->name, filename))
					{
						// found it!
						// com_filesize = pk3->buildBuffer[i].filelen;
						file_from_pk3 = 1;
						if (path_id)
							*path_id = search->path_id;
						if (handle)
						{
							Sys_FileOpenRead(pk3->pakFilename, handle);

							fsh_ifstream[*handle].handleFiles.file.z = unzReOpen(pk3->pakFilename, pk3->handle);
							if (fsh_ifstream[*handle].handleFiles.file.z == NULL)
							{
								Sys_Error("Couldn't reopen %s", pk3->pakFilename);
							}
							else
							{
								fsh_ifstream[*handle].handleFiles.file.z = pk3->handle;
							}

							Q_strncpy(fsh_ifstream[*handle].name, filename, sizeof(fsh_ifstream[*handle].name));
							fsh_ifstream[*handle].zipFile = true;
							zfi = (unz_s*)fsh_ifstream[*handle].handleFiles.file.z;
							// in case the file was new
							temp = zfi->ifstr;
							// set the file position in the zip file (also sets the current file info)
							unzSetCurrentFileInfoPosition(pk3->handle, pakFile->pos);
							// copy the file info into the unzip structure
							memcpy(zfi, pk3->handle, sizeof(unz_s));
							// we copy this back into the structure
							zfi->ifstr = temp;
							// open the file in the zip
							unzOpenCurrentFile(fsh_ifstream[*handle].handleFiles.file.z);
							fsh_ifstream[*handle].zipFilePos = pakFile->pos;

							com_filesize = zfi->cur_file_info.uncompressed_size;

							*pk3_file = fsh_ifstream[*handle].handleFiles.file.z;

							return zfi->cur_file_info.uncompressed_size;
						}
						else if (file)
						{ /* open a new file on the pakfile */

							/**file = fopen(pk3->pakFilename, "rb");
							if (*file)
								fseek(*file, pakFile->pos, SEEK_SET);*/

							int hndl = g_FileSystem->FS_HandleForFile();

							file->open(pk3->pakFilename, cxxifstream::in | cxxifstream::binary);

							fsh_ifstream[hndl].handleFiles.file.z = unzReOpen(pk3->pakFilename, pk3->handle);
							fsh_ifstream[hndl].handleFiles.file.z = pk3->handle;

							Q_strncpy(fsh_ifstream[hndl].name, filename, sizeof(fsh_ifstream[hndl].name));
							fsh_ifstream[hndl].zipFile = true;
							zfi = (unz_s*)pk3->handle;
							// in case the file was new
							temp = zfi->ifstr;
							// set the file position in the zip file (also sets the current file info)
							if ((unzSetCurrentFileInfoPosition(pk3->handle, pakFile->pos)) != UNZ_OK)
							{
								Sys_Error("CCommon::COM_FindFile_IFStream: unzSetCurrentFileInfoPosition_IFStream failed\n");
								return -1;
							}

							// copy the file info into the unzip structure
							memcpy(zfi, pk3->handle, sizeof(unz_s));
							// we copy this back into the structure
							zfi->ifstr = temp;
							// open the file in the zip
							unzOpenCurrentFile(file);
							fsh_ifstream[hndl].zipFilePos = pakFile->pos;

							com_filesize = zfi->cur_file_info.uncompressed_size;

							*pk3_file = fsh_ifstream[hndl].handleFiles.file.z;

							return com_filesize;
						}
						else /* for COM_FileExists() */
						{
							return com_filesize;
						}
					}

					pakFile = pakFile->next;
				} while (pakFile != nullptr);
			}
		}
		else
		{
			if (!registered.value)
			{ /* if not a registered version, don't ever go beyond base */
				if (strchr(filename, '/') || strchr(filename, '\\'))
					continue;
			}

			snprintf(netpath, sizeof(netpath), "%s/%s", search->filename, filename);
			if (!(Sys_FileType(netpath) & FS_ENT_FILE))
				continue;

			if (path_id)
				*path_id = search->path_id;
			if (handle)
			{
				com_filesize = Sys_FileOpenRead(netpath, &i);
				*handle = i;
				return com_filesize;
			}
			else if (file)
			{
				file->open(netpath, cxxifstream::binary | cxxifstream::in);
				com_filesize = COM_filelength_FStream(file);
				return com_filesize;
			}
			else
			{
				return 0; /* dummy valid value for COM_FileExists() */
			}
		}
	}

	Sys_Printf("FindFile: can't find %s\n", filename);

	if (handle)
		*handle = -1;
	if (file)
		file->close();
	com_filesize = -1;
	return com_filesize;
}

/*
===========
Missi: COM_FindFile_OFStream

Like COM_FindFile_IFStream, but without support for pakfiles. Mainly used to write
things to the game directory on disk. (8/29/2024)
===========
*/
void CCommon::COM_FindFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id)
{
	file_from_pak = 0;
	file_from_pk3 = 0;

	char fullpath[MAX_OSPATH];
	snprintf(fullpath, sizeof(fullpath), "%s", filename);

	Q_FixSlashes(fullpath, sizeof(fullpath));

	file->open(fullpath, cxxofstream::binary | cxxofstream::out | cxxofstream::trunc);
}

/*
===========
COM_FindFile_VPK

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
cxxifstream* CCommon::COM_FindFile_VPK(const char* filename, uintptr_t* path_id)
{
	file_from_pak = 0;
	file_from_pk3 = 0;

	//
	// search through the path, one element at a time
	//
	const int idx = FindVPKIndexForFileAmongstLoadedVPKs(filename);
	const VPKDirectoryEntry* entry = FindVPKFileAmongstLoadedVPKs(filename);

	if (idx != -1)
	{
		cxxifstream* file = loaded_vpks[idx][entry->ArchiveIndex+1];
		file->seekg(entry->EntryOffset, cxxifstream::beg);
		file->clear();

		com_filesize = entry->EntryLength;
		return file;
	}

	delete[] entry;

	Sys_Printf("FindFile: can't find %s\n", filename);

	com_filesize = -1;
	return nullptr;
}

/*
================
COM_filelength
================
*/
long CCommon::COM_filelength(FILE* f)
{
	long		pos, end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

/*
================
COM_filelength
================
*/
size_t CCommon::COM_filelength_FStream(cxxifstream* f)
{
	size_t		end;

	f->seekg(0,	cxxifstream::end);
	end = f->tellg();
	f->seekg(0, cxxifstream::beg);

	return end;
}

/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int CCommon::COM_OpenFile (const char *filename, int *handle, uintptr_t* path_id)
{
	return COM_FindFile (filename, handle, NULL, path_id);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int CCommon::COM_FOpenFile (const char *filename, FILE **file, uintptr_t* path_id)
{
	return COM_FindFile (filename, nullptr, file, path_id);
}

/*
===========
COM_FOpenFile_IFStream

If the requested file is inside a packfile, a new cxxifstream* will be opened
into the file.
===========
*/
int CCommon::COM_FOpenFile_IFStream(const char* filename, cxxifstream* file, uintptr_t* path_id, unzFile* pk3_file)
{
	return COM_FindFile_IFStream(filename, nullptr, file, path_id, pk3_file);
}

/*
===========
COM_FOpenFile_OFStream

Like COM_FOpenFile_IFStream, but without support for pakfiles. Mainly used to write
things to the game directory on disk.
===========
*/
void CCommon::COM_FOpenFile_OFStream(const char* filename, cxxofstream* file, uintptr_t* path_id)
{
	return COM_FindFile_OFStream(filename, file, path_id);
}

/*
===========
COM_FOpenFile_VPK

If the requested file is inside a VPK, a new cxxifstream* will be opened
into the file.
===========
*/
cxxifstream* CCommon::COM_FOpenFile_VPK(const char* filename, uintptr_t* path_id)
{
	return COM_FindFile_VPK(filename, path_id);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void CCommon::COM_CloseFile (int h)
{
	searchpath_t    *s;
	
	for (s = com_searchpaths ; s ; s=s->next)
		if (s->pack && s->pack->handle == h)
			return;
			
	Sys_FileClose (h);
}

//int             loadsize;
//byte* loadbuf;

byte* CCommon::COM_LoadMallocFile_TextMode_OSPath(const char* path, long* len_out)
{
	cxxifstream f;
	byte* data;
	long	len, actuallen;

	// Missi: copied from QuakeSpasm (6/7/2024)
	f.open(path);
	if (!f.is_open())
		return NULL;

	len = COM_filelength_FStream(&f);
	if (len < 0)
	{
		f.close();
		return NULL;
	}

	data = (byte*)malloc(len + 1);
	if (data == NULL)
	{
		f.close();
		return NULL;
	}

	// (actuallen < len) if CRLF to LF translation was performed
	f.read((char*)data, len);
	f.clear();
	actuallen = f.tellg();
	if (!f.good())
	{
		f.close();
		free(data);
		return NULL;
	}
	data[actuallen] = '\0';

	if (len_out != NULL)
		*len_out = actuallen;
	f.close();
	return data;
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t* CCommon::COM_LoadPackFile (char *packfile)
{
	dpackheader_t			header;
	int                     i = 0;
	packfile_t              *newfiles = NULL;
	int                     numpackfiles = 0;
	pack_t                  *pack = NULL;
	int                     packhandle = 0;
	static dpackfile_t      info[MAX_FILES_IN_PACK];
	unsigned short          crc = 0;

	if (Sys_FileOpenRead (packfile, &packhandle) == -1)
	{
		Con_Printf ("Couldn't open %s\n", packfile);
		return NULL;
	}
	Sys_FileRead (packhandle, &header, sizeof(header));
	if (header.id[0] != 'P' || header.id[1] != 'A'
	|| header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %i files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = true;    // not the original file

	newfiles = g_MemCache->Hunk_AllocName<packfile_t>(numpackfiles * sizeof(packfile_t), "packfile");

	Sys_FileSeek (packhandle, header.dirofs);
	Sys_FileRead (packhandle, info, header.dirlen);

// crc the directory to check for modifications
	g_CRCManager->CRC_Init (&crc);
	for (i=0 ; i<header.dirlen ; i++)
		g_CRCManager->CRC_ProcessByte (&crc, ((byte *)info)[i]);
	if (crc != PAK0_CRC)
		com_modified = true;

// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		Q_strcpy (newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = g_MemCache->Hunk_Alloc<pack_t>(sizeof (pack_t));
	Q_strcpy (pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	
	Con_PrintColor (TEXT_COLOR_GREEN, "Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 

Missi: copied from QuakeSpasm (1/4/2023)
================
*/
void CCommon::COM_AddGameDirectory (const char *dir)
{
	int                     i = 0;
	searchpath_t			*search = nullptr;
	pack_t                  *pak = nullptr;
	pack_pk3_t				*pk3 = nullptr;
	char                    pakfile[1024];
	uintptr_t				path_id = 0;
	bool                    been_here = false;

	Q_strncpy (com_gamedir, dir, sizeof(com_gamedir));

	memset(pakfile, 0, sizeof(pakfile));

	// assign a path_id to this game directory
	if (com_searchpaths)
		path_id = com_searchpaths->path_id << 1;
	else	path_id = 1U;

//
// add the directory to the search path
//
	search = mainzone->Z_Malloc<searchpath_t>(sizeof(searchpath_t));
	search->path_id = path_id;
	Q_strncpy (search->filename, dir, sizeof(search->filename));
	search->next = com_searchpaths;
	com_searchpaths = search;

//
// add any pak files in the format pak0.pak pak1.pak, ...
//
	for (i=0 ; ; i++)
	{
		snprintf (pakfile, sizeof(pakfile), "%s/PAK%i.PAK", dir, i);
		pak = COM_LoadPackFile (pakfile);

		if (!pak)
			break;

		search = mainzone->Z_Malloc<searchpath_t>(sizeof(searchpath_t));
		search->path_id = path_id;
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;
	}

	char pk3_folder[MAX_OSPATH];
	snprintf(pk3_folder, sizeof(pk3_folder), "%s/pk3", dir);

	if (!fs::exists(pk3_folder))
		return;

	for (const auto& file : fs::directory_iterator(pk3_folder))
	{
		if (!file.is_regular_file() || file.is_directory())
			continue;

		size_t ext = file.path().string().find(".pk3");

		if (ext == cxxstring::npos)
			continue;

		cxxstring noext = file.path().string().erase(ext, 4);

		pk3 = g_FileSystem->FS_LoadZipFile(file.path().string().c_str(), noext.c_str());
		
		if (!pk3)
			continue;

		Q_strncpy(pk3->pakGamename, GAMENAME, sizeof(pk3->pakGamename));

		search = mainzone->Z_Malloc<searchpath_t>(sizeof(searchpath_t));
		search->path_id = path_id;
		search->pk3 = pk3;
		search->next = com_searchpaths;
		com_searchpaths = search;

		Con_PrintColor(TEXT_COLOR_GREEN, "Added PK3 file %s (%d files)\n", file.path().string().c_str(), pk3->numfiles);
	}

//
// add the contents of the parms.txt file to the end of the command line
//

}

/*
================
COM_InitFilesystem
================
*/
void CCommon::COM_InitFilesystem ()
{
	int				i;
	char			basedir[MAX_OSPATH] = "";
	searchpath_t	*search = NULL;

	memset(com_gamedir, 0, sizeof(com_gamedir));
	memset(com_cachedir, 0, sizeof(com_cachedir));

//
// -basedir <path>
// Overrides the system supplied base directory (under GAMENAME)
//
	i = COM_CheckParm ("-basedir");
	if (i && i < com_argc-1)
		Q_strcpy (basedir, com_argv[i+1]);
	else
		Q_strcpy (basedir, host->host_parms.basedir);

	if (basedir[Q_strlen(basedir)-1] == '\\' || basedir[Q_strlen(basedir)-1] == '/')
		basedir[Q_strlen(basedir)-1] = '\0';

//
// -cachedir <path>
// Overrides the system supplied cache directory (NULL or /qcache)
// -cachedir - will disable caching.
//
	i = COM_CheckParm ("-cachedir");
	if (i && i < com_argc-1)
	{
		if (com_argv[i+1][0] == '-')
			com_cachedir[0] = 0;
		else
			Q_strcpy (com_cachedir, com_argv[i+1]);
	}
	else if (host->host_parms.cachedir)
		Q_strcpy (com_cachedir, host->host_parms.cachedir);
	else
		com_cachedir[0] = 0;

//
// start up with GAMENAME by default (id1)
//
	COM_AddGameDirectory (va("%s/" GAMENAME, basedir) );

	if (COM_CheckParm ("-rogue"))
		COM_AddGameDirectory (va("%s/rogue", basedir) );
	if (COM_CheckParm ("-hipnotic"))
		COM_AddGameDirectory (va("%s/hipnotic", basedir) );

//
// -game <gamedir>
// Adds basedir/gamedir as an override game
//
	i = COM_CheckParm ("-game");
	if (i && i < com_argc-1)
	{
		com_modified = true;

		// Missi: if it starts with a forward slash (on Linux/Mac), a quotation mark, or a drive letter (Windows) then it's an absolute path (6/2/2024)
		const bool absolute = ((com_argv[i + 1][0] > 64) && (com_argv[i + 1][0] < 91) && (com_argv[i + 1][1] == ':'));

		if (!absolute)
			COM_AddGameDirectory (va("%s/%s", basedir, com_argv[i+1]));
		else
		{
			const char* absPath = com_argv[i + 1];
			char path[MAX_QPATH];

#ifndef __linux__
			Q_FixQuotes(path, absPath, sizeof(path));
			Q_FixSlashes(path, sizeof(path));

			COM_AddGameDirectory(va("%s", path));
#else
			COM_AddGameDirectory(va("%s", absPath));
#endif
		}
	}

//
// -path <dir or packfile> [<dir or packfile>] ...
// Fully specifies the exact serach path, overriding the generated one
//
	i = COM_CheckParm ("-path");
	if (i)
	{
		com_modified = true;
		com_searchpaths = nullptr;
		while (++i < com_argc)
		{
			if (!com_argv[i] || com_argv[i][0] == '+' || com_argv[i][0] == '-')
				break;
			
			search = g_MemCache->Hunk_Alloc<searchpath_t>(sizeof(searchpath_t));
			if ( !strcmp(COM_FileExtension(com_argv[i]), "pak") )
			{
				search->pack = COM_LoadPackFile (com_argv[i]);
				if (!search->pack)
					Sys_Error ("Couldn't load packfile: %s", com_argv[i]);
			}
			else
				Q_strcpy (search->filename, com_argv[i]);
			search->next = com_searchpaths;
			com_searchpaths = search;
		}
	}

#ifdef _DEBUG
	for (int l = 0; l < com_argc; l++)
	{
		Sys_Printf("com_argv[%d]: %s\n", l, com_argv[l]);
	}
#endif

	if (COM_CheckParm ("-proghack"))
		proghack = true;
}

/* Missi: copied from QuakeSpasm (1/6/2023)
 * The following FS_*() stdio replacements are necessary if one is
 * to perform non-sequential reads on files reopened on pak files
 * because we need the bookkeeping about file start/end positions.
 * Allocating and filling in the fshandle_t structure is the users'
 * responsibility when the file is initially opened. */

size_t CFileSystem::FS_fread(void* ptr, size_t size, size_t nmemb, fshandle_t* fh)
{
	long byte_size;
	long bytes_read;
	size_t nmemb_read;

	if (!fh) {
		errno = EBADF;
		return 0;
	}
	if (!ptr) {
		errno = EFAULT;
		return 0;
	}
	if (!size || !nmemb) {	/* no error, just zero bytes wanted */
		errno = 0;
		return 0;
	}

	byte_size = nmemb * size;
	if (byte_size > fh->length - fh->pos)	/* just read to end */
		byte_size = fh->length - fh->pos;
	bytes_read = fread(ptr, 1, byte_size, fh->file);
	fh->pos += bytes_read;

	/* fread() must return the number of elements read,
	 * not the total number of bytes. */
	nmemb_read = bytes_read / size;
	/* even if the last member is only read partially
	 * it is counted as a whole in the return value. */
	if (bytes_read % size)
		nmemb_read++;

	return nmemb_read;
}

int CFileSystem::FS_fseek(fshandle_t* fh, long offset, int whence)
{
	/* I don't care about 64 bit off_t or fseeko() here.
	 * the quake/hexen2 file system is 32 bits, anyway. */
	int ret;

	if (!fh) {
		errno = EBADF;
		return -1;
	}

	/* the relative file position shouldn't be smaller
	 * than zero or bigger than the filesize. */
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += fh->pos;
		break;
	case SEEK_END:
		offset = fh->length + offset;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	if (offset > fh->length)	/* just seek to end */
		offset = fh->length;

	ret = fseek(fh->file, fh->start + offset, SEEK_SET);
	if (ret < 0)
		return ret;

	fh->pos = offset;
	return 0;
}

int CFileSystem::FS_fclose(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fclose(fh->file);
}

long CFileSystem::FS_ftell(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->pos;
}

void CFileSystem::FS_rewind(fshandle_t* fh)
{
	if (!fh) return;
	clearerr(fh->file);
	fseek(fh->file, fh->start, SEEK_SET);
	fh->pos = 0;
}

/*
================
return a hash value for the filename
================
*/
long CFileSystem::FS_HashFileName(const char* fname, int hashSize) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter == '.') break;				// don't include extension
		if (letter == '\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize - 1);
	return hash;
}

int CFileSystem::FS_HandleForFile()
{
	for (int i = 0; i < MAX_HANDLES; i++)
	{
		if (!sys_handles[i])
		{
			return i;
		}
	}

	return -1;
}

int CFileSystem::FS_feof(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	if (fh->pos >= fh->length)
		return -1;
	return 0;
}

int CFileSystem::FS_ferror(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return ferror(fh->file);
}

int CFileSystem::FS_fgetc(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return EOF;
	}
	if (fh->pos >= fh->length)
		return EOF;
	fh->pos += 1;
	return fgetc(fh->file);
}

char* CFileSystem::FS_fgets(char* s, int size, fshandle_t* fh)
{
	char* ret;

	if (FS_feof(fh))
		return NULL;

	if (size > (fh->length - fh->pos) + 1)
		size = (fh->length - fh->pos) + 1;

	ret = fgets(s, size, fh->file);
	fh->pos = ftell(fh->file) - fh->start;

	return ret;
}

long CFileSystem::FS_filelength(fshandle_t* fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->length;
}

/*
=================
FS_LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
pack_pk3_t* CFileSystem::FS_LoadZipFile(const char* zipfile, const char* basename)
{
	fileInPack_t*	buildBuffer = {};
	pack_pk3_t*		pack = {};
	unzFile			uf = {};
	int				err = 0;
	unz_global_info gi = {};
	char			filename_inzip[MAX_ZPATH] = {};
	unz_file_info	file_info = {};
	int				i = 0, len = 0;
	long			hash = 0;
	int				fs_numHeaderLongs = 0;
	int*			fs_headerLongs = { 0 };
	char*			namePtr = nullptr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo(uf, &gi);

	if (err != UNZ_OK)
		return NULL;

	fs_packFiles += gi.number_entry;

	len = 0;

	unzGoToFirstFile(uf);
	for (i = 0; i < (int)gi.number_entry; i++)
	{

		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = mainzone->Z_Malloc<fileInPack_t>((gi.number_entry * sizeof(fileInPack_t)) + len);
	namePtr = ((char*)buildBuffer) + gi.number_entry * sizeof(fileInPack_t);
	fs_headerLongs = mainzone->Z_Malloc<int>(gi.number_entry * sizeof(int));

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1) {
		if (i > (int)gi.number_entry) {
			break;
		}
	}

	pack = mainzone->Z_Malloc<pack_pk3_t>(sizeof(pack_pk3_t) + i * sizeof(fileInPack_t*));
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t**)(((char*)pack) + sizeof(pack_pk3_t));
	for (i = 0; i < pack->hashSize; i++) {
		pack->hashTable[i] = NULL;
	}

	Q_strncpy(pack->pakFilename, zipfile, sizeof(pack->pakFilename));
	Q_strncpy(pack->pakBasename, basename, sizeof(pack->pakBasename));

	// strip .pk3 if needed
	if (strlen(pack->pakBasename) > 4 && !Q_stricmp(pack->pakBasename + strlen(pack->pakBasename) - 4, ".pk3")) {
		pack->pakBasename[strlen(pack->pakBasename) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for (i = 0; i < (int)gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		if (file_info.uncompressed_size > 0) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		Q_strlwr(filename_inzip);
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		Q_strcpy(buildBuffer[i].name, filename_inzip);
		namePtr += strlen(filename_inzip) + 1;
		// store the file position in the zip
		unzGetCurrentFileInfoPosition(uf, &buildBuffer[i].pos);
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum(fs_headerLongs, 4 * fs_numHeaderLongs);
	pack->pure_checksum = Com_BlockChecksumKey(fs_headerLongs, 4 * fs_numHeaderLongs, LittleLong(fs_checksumFeed));
	pack->checksum = LittleLong(pack->checksum);
	pack->pure_checksum = LittleLong(pack->pure_checksum);

	mainzone->Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}
