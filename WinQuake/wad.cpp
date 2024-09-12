/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schimedeberg

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
// wad.c

#include "quakedef.h"
#include "wad.h"

wadinfo_t*          loaded_wads[MAX_LOADED_WADS];
int			        wad_numlumps[MAX_LOADED_WADS];
lumpinfo_t*         wad_lumps[MAX_LOADED_WADS];
byte*               wad_base[MAX_LOADED_WADS];
const char*         wad_names[MAX_LOADED_WADS];


/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (const char *in, char *out)
{
	int		i;
	int		c;
	
	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;
			
		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}
	
	for ( ; i< 16 ; i++ )
		out[i] = 0;
}

int W_GetLoadedWadFile(const char* filename)
{
    for (int i = 0; i < MAX_LOADED_WADS; i++)
    {
        if (!Q_strcmp(wad_names[i], filename))
            return i;
    }

    return -1;
}

/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (const char *filename)
{
	lumpinfo_t		*lump_p = nullptr;
	wadinfo_t		*header = nullptr;
	unsigned		i = 0;
    int             j = 0;
    bool            loaded = false;
	int				infotableofs = 0;
	
    for (j = 0; j < MAX_LOADED_WADS; j++)
    {
        if (wad_base[j])
            continue;

        wad_base[j] = COM_LoadHunkFile<byte> (filename, NULL);
        loaded = true;
        break;
    }

    if (!wad_base[j] || !loaded)
	{
		Con_Warning ("W_LoadWadFile: couldn't load %s", filename);
		return;
	}

    header = (wadinfo_t *)wad_base[j];

	if (!header)
		return;
	
	if (header->identification[0] != 'W'
	|| header->identification[1] != 'A'
	|| header->identification[2] != 'D'
	|| header->identification[3] != '2')
		Sys_Error ("Wad file %s doesn't have WAD2 id\n",filename);
		
    loaded_wads[j] = header;
    wad_names[j] = filename;

    wad_numlumps[j] = LittleLong(header->numlumps);
	infotableofs = LittleLong(header->infotableofs);
    wad_lumps[j] = (lumpinfo_t *)(wad_base[j] + infotableofs);
	
    for (i=0, lump_p = wad_lumps[j] ; (int)i < wad_numlumps[j] ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName (lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
            SwapPic ( (qpicbuf_t *)(wad_base[j] + lump_p->filepos));
	}
}

void W_LoadGoldSrcWadFiles()
{
    W_LoadWadFiles_GoldSrc();
}

/*
====================
W_LoadWadFile_GoldSrc
====================
*/
void W_LoadWadFiles_GoldSrc()
{
    lumpinfo_t      *lump_p = nullptr;
    wadinfo_t		*header = nullptr;
    int             j = 0;
    unsigned		i = 0;
    int				infotableofs = 0;
    char*           name[MAX_LOADED_WADS] = {};

    if (!fs::exists(g_Common->com_gamedir) || fs::is_empty(g_Common->com_gamedir))
        return;

    for (const auto& entry : fs::directory_iterator(g_Common->com_gamedir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().string().find(".wad") != cxxstring::npos)
        {
            size_t pos = entry.path().string().find_last_of("\\");

            if (pos == cxxstring::npos)
            {
                pos = entry.path().string().find_last_of("/");
            }

            cxxstring sanitized = entry.path().string().erase(0, pos+1);

            name[j] = new char[256];

            Q_strncpy(name[j], sanitized.c_str(), 256);
            j++;
        }
    }

    for (j = 0; j < MAX_LOADED_WADS; j++)
    {
        if (wad_base[j])
            continue;

        if (!name[j])
            break;

        wad_base[j] = COM_LoadHunkFile<byte>(name[j], NULL);

        header = (wadinfo_t*)wad_base[j];
        wad_names[j] = name[j];

        if (!header)
            return;

        if (header->identification[0] != 'W'
            || header->identification[1] != 'A'
            || header->identification[2] != 'D'
            || header->identification[3] != '3')
            continue;

        wad_numlumps[j] = LittleLong(header->numlumps);

        infotableofs = LittleLong(header->infotableofs);
        wad_lumps[j] = (lumpinfo_t*)(wad_base[j] + infotableofs);

        Con_PrintColor(TEXT_COLOR_GREEN, "Added WAD3 file %s/%s (%d files)\n", g_Common->com_gamedir, name[j], wad_numlumps[j]);

        for (i = 0, lump_p = wad_lumps[j]; (int)i < wad_numlumps[j]; i++, lump_p++)
        {
            lump_p->filepos = LittleLong(lump_p->filepos);
            lump_p->size = LittleLong(lump_p->size);
            W_CleanupName(lump_p->name, lump_p->name);
        }
    }

    delete[] *name;
}

int W_GetExternalTextureWadFile (const char* name)
{
    for (int i = 0; i < MAX_LOADED_WADS; i++)
    {
        lumpinfo_t* lump = wad_lumps[i];

        if (!lump)
            continue;

        while (lump->name[0])
        {
            if (lump && !Q_strcmp(lump->name, name))
            {
                return i;
            }

            lump++;
        }
    }

    Con_Printf("Couldn't find wad file for texture %s\n", name);

    return -1;
}

goldsrc_qpic_t*  W_GetExternalQPic (const char* name)
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
                return (goldsrc_qpic_t*)(wad_base[i] + lump->filepos);
            }

            lump++;
        }
    }

    Con_Printf("Couldn't find texture %s\n", name);

    return nullptr;
}


int W_GetExternalTextureLumpPos(const char* name)
{
    for (int i = 0; i < MAX_LOADED_WADS; i++)
    {
        if (!wad_lumps[i])
            continue;

        lumpinfo_t* lump = wad_lumps[i];

        if (!lump)
            continue;

        while (lump->name[0])
        {
            if (!Q_strcmp(lump->name, name))
            {
                return lump->filepos;
            }

            lump++;
        }
    }

    Con_Printf("Couldn't find texture %s\n", name);

    return -1;
}

lumpinfo_t* W_GetExternalTextureLumpInfo(const char* name)
{
    for (int i = 0; i < MAX_LOADED_WADS; i++)
    {
        if (!wad_lumps[i])
            continue;

        lumpinfo_t* lump = wad_lumps[i];

        if (!lump)
            continue;

        int pos = 0;

        while (lump->name[0])
        {
            if (!Q_strcmp(lump->name, name))
            {
                return lump;
            }

            pos++;
            lump++;
        }
    }

    Con_Printf("Couldn't find texture %s\n", name);

    return nullptr;
}

/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t	*W_GetLumpinfo (const char *name, const char* wadname)
{
	int		i;
	lumpinfo_t	*lump_p;
	char	clean[16];
	
    int wad = W_GetLoadedWadFile(wadname);

    if (wad == -1)
        return nullptr;

    W_CleanupName (name, clean);
	
    for (lump_p = wad_lumps[wad], i=0 ; i < wad_numlumps[wad] ; i++,lump_p++)
	{
        if (!Q_strcmp(clean, lump_p->name))
			return lump_p;
	}
	
	Con_Warning ("W_GetLumpinfo: %s not found", name);
	return NULL;
}

/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t* W_GetLumpinfo_GoldSrc(const char* name, const char* wadname)
{
    int		i;
    lumpinfo_t* lump_p;
    char	clean[16];

    int wad = W_GetLoadedWadFile(wadname);

    if (wad == -1)
        return nullptr;

    W_CleanupName(name, clean);

    for (lump_p = wad_lumps[wad], i = 0; i < wad_numlumps[wad]; i++, lump_p++)
    {
        if (!Q_strcmp(clean, lump_p->name))
            return lump_p;
    }

    Con_Warning("W_GetLumpinfo: %s not found", name);
    return NULL;
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic (qpicbuf_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}

void SwapPic_GoldSrc (goldsrc_qpic_t *pic)
{
    pic->width = LittleLong(pic->width);
    pic->height = LittleLong(pic->height);
    pic->data = pic->data;
    pic->colors_used = LittleShort(pic->colors_used);

    for (int i = 0; i < pic->colors_used; i++)
    {
        pic->lbmpalette[i][0] = LittleLong(pic->lbmpalette[i][0]);
        pic->lbmpalette[i][1] = LittleLong(pic->lbmpalette[i][1]);
        pic->lbmpalette[i][2] = LittleLong(pic->lbmpalette[i][2]);
    }
}

CQuakePic::CQuakePic() : height(0), width(0)
{
	memset(&datavec, 0, sizeof(&datavec));
}

CQuakePic::CQuakePic(byte& mem) : height(0), width(0)
{
	memset(&datavec, 0, sizeof(&datavec));
}

CQuakePic::CQuakePic(const CQuakePic& src) : height(0), width(0)
{
	memset(&datavec, 0, sizeof(&datavec));
}
