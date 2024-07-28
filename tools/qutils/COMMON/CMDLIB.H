// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

typedef unsigned char byte;

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strupr (char *in);
char *strlower (char *in);
int Q_strncasecmp(const char* s1, const char* s2, size_t n);
int Q_strcasecmp (const char *s1, const char *s2);
void Q_getwd (char *out);

int filelength (FILE *f);
int	FileTime (char *path);

void	Q_mkdir (const char *path);

extern	char		qdir[1024];
extern	char		gamedir[1024];
void SetQdirFromPath (const char *path);
const char *ExpandPath (const char *path);
const char *ExpandPathAndArchive (const char *path);


double I_FloatTime (void);

void	Error (const char *error, ...);
int		CheckParm (const char *check);

FILE	*SafeOpenWrite (const char *filename);
FILE	*SafeOpenRead (const char *filename);
void	SafeRead (FILE *f, void *buffer, int count);
void	SafeWrite (FILE *f, void *buffer, int count);

int		LoadFile (const char *filename, void **bufferptr);
void	SaveFile (const char *filename, void *buffer, int count);

void 	DefaultExtension (char *path, const char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest);
void	ExtractFileExtension (char *path, char *dest);

int 	ParseNum (char *str);

short	BigShort (short l);
short	LittleShort (short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);


char *COM_Parse (char *data);

extern	char		com_token[1024];
extern	bool		com_eof;

char *copystring(char *s);


void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);

void	CreatePath (const char *path);
void CopyFile (const char *from, const char *to);

extern	bool			archive;
extern	char			archivedir[1024];


#endif
