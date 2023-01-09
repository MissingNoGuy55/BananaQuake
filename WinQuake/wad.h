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
// wad.h

//===============
//   TYPES
//===============

#ifndef WAD_H
#define WAD_H

#define	CMP_NONE		0
#define	CMP_LZSS		1

#define	TYP_NONE		0
#define	TYP_LABEL		1

#define	TYP_LUMPY		64				// 64 + grab command number
#define	TYP_PALETTE		64
#define	TYP_QTEX		65
#define	TYP_QPIC		66
#define	TYP_SOUND		67
#define	TYP_MIPTEX		68

struct CQuakePic
{
public:

	CQuakePic();
	CQuakePic(byte& mem);
	CQuakePic(const CQuakePic& src);

	/*CQuakePic& operator=(void* src);
	CQuakePic& operator=(const void* src);*/

	int			width;
	int			height;
	//byte		data[1];		// Missi: Array size has been treated as a constant expression in C++ for a long time. 
								// While C variable-length arrays are supported in C++17 and beyond, they're extremely 
								// susceptible to corruption and overruns from other memory.
								// See the comment in CGLRenderer::Draw_PicFromWad (12/8/2022)
	CQVector<byte>	datavec;

};

typedef struct qpicbuf_s
{
	int		width;
	int		height;
	byte	data[1];
} qpicbuf_t;

struct CQuakePicVector
{
	CQVector<byte>	data;
};

typedef struct
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
} wadinfo_t;

typedef struct
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} lumpinfo_t;

extern	int			wad_numlumps;
extern	lumpinfo_t	*wad_lumps;
extern	byte		*wad_base;

void	W_LoadWadFile (const char *filename);
void	W_CleanupName (const char *in, char *out);
lumpinfo_t	*W_GetLumpinfo (const char *name);

template<typename T>
T	*W_GetLumpName (const char *name);
template<typename T>
T	*W_GetLumpNum (int num);

void	SwapPic(qpicbuf_t* pic);

template<typename T>
T* W_GetLumpName(const char* name)
{
	lumpinfo_t* lump;

	lump = W_GetLumpinfo(name);

	return (T*)(wad_base + lump->filepos);
}

template<typename T>
T* W_GetLumpNum(int num)
{
	lumpinfo_t* lump;

	if (num < 0 || num > wad_numlumps)
		Sys_Error("W_GetLumpNum: bad number: %i", num);

	lump = wad_lumps + num;

	return (T*)(wad_base + lump->filepos);
}

#endif

