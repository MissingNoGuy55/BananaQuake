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

enum srcformat_t
{
	SRC_NONE,
	SRC_INDEXED,
	SRC_LIGHTMAP,
	SRC_RGBA,
	SRC_INDEXED_WAD
};

struct CQuakePic
{
public:

	CQuakePic();
	CQuakePic(byte& mem);
	~CQuakePic() { datavec.Clear(); }

	int			width;
	int			height;
    CQVector<byte>	datavec;

private:

	CQuakePic(const CQuakePic& src);

};

class CQuakeTGAPic
{
public:

	~CQuakeTGAPic() { datavec.Clear(); }

    int     width;
    int     height;

    CQVector<void*> datavec;
};

#define MAX_WAD_TEXWIDTH 4096
#define MAX_WAD_TEXHEIGHT 4096

constexpr const char* GFX_WAD   =   "gfx.wad";

typedef struct qpicbuf_s
{
    int		width;
    int		height;
    byte	data[1];
} qpicbuf_t;

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
	bool		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} lumpinfo_t;

typedef struct goldsrc_qpic_s
{
    int width, height;
    byte* data; //Image is stored as 8-bit numbers of colors in palette
    short colors_used; //Number of colors in palette (can't be more than 256)
    byte lbmpalette[256][3]; //8-bit RGB palette data
} goldsrc_qpic_t;

constexpr unsigned int      MAX_LOADED_WADS = 32;

extern  wadinfo_t*          loaded_wads[MAX_LOADED_WADS];
extern	int			        wad_numlumps[MAX_LOADED_WADS];
extern	lumpinfo_t*         wad_lumps[MAX_LOADED_WADS];
extern	byte*               wad_base[MAX_LOADED_WADS];
extern	const char*         wad_names[MAX_LOADED_WADS];

int                 W_GetLoadedWadFile(const char* filename);
void	            W_LoadWadFile (const char *filename);
void                W_LoadWadFile_GoldSrc (const char *filename);
void                W_LoadGoldSrcWadFiles();
void	            W_CleanupName (const char *in, char *out);
lumpinfo_t*         W_GetLumpinfo (const char *name, const char* wadname);
lumpinfo_t*         W_GetLumpinfo_GoldSrc(const char* name, const char* wadname);

template<typename T>
T* W_GetLumpName (const char *name, const char* wadname);

template<typename T>
T* W_GetLumpName_GoldSrc(const char* name, const char* wadname);

template<typename T>
T* W_GetLumpNum (const char* name, int num);

template<typename T>
T* W_GetExternalTexture (const char* name);

int W_GetExternalTextureWadFile(const char* name);
int W_GetExternalTextureLumpPos(const char* name);
lumpinfo_t* W_GetExternalTextureLumpInfo(const char* name);
goldsrc_qpic_t*  W_GetExternalQPic (const char* name);

void	SwapPic(qpicbuf_t* pic);
void    SwapPic_GoldSrc (goldsrc_qpic_t *pic);

template<typename T>
T* W_GetLumpName_GoldSrc(const char* name, const char* wadname)
{
	lumpinfo_t* lump;

	lump = W_GetLumpinfo_GoldSrc(name, wadname);

	if (!lump)
		return nullptr;

	int wad = W_GetLoadedWadFile(wadname);

	return (T*)(wad_base[wad] + lump->filepos);
}

template<typename T>
T* W_GetLumpName(const char* name, const char* wadname)
{
	lumpinfo_t* lump = nullptr;

    lump = W_GetLumpinfo(name, wadname);

	if (!lump)
		return nullptr;

    int wad = W_GetLoadedWadFile(wadname);

    return (T*)(wad_base[wad] + lump->filepos);
}

template<typename T>
T* W_GetLumpNum(const char* name, int num)
{
	lumpinfo_t* lump;

    int wad = W_GetLoadedWadFile(name);

    if (num < 0 || num > wad_numlumps[wad])
		Sys_Error("W_GetLumpNum: bad number: %i", num);

    lump = wad_lumps[wad] + num;

    return (T*)(wad_base[wad] + lump->filepos);
}

template<typename T>
T* W_GetExternalTexture (const char* name)
{
    for (int i = 0; i < MAX_LOADED_WADS; i++)
    {
        if (!wad_lumps[i])
            continue;

        lumpinfo_t* lump = wad_lumps[i];

        while (lump->name[0])
        {
            if (lump && !Q_strcmp(lump->name, name))
            {
                return (T*)(wad_base[i] + lump->filepos);
            }

            lump++;
        }
    }

    Con_Printf("Couldn't find texture %s\n", name);

    return nullptr;
}

#endif

