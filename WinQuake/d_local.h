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
// d_local.h:  private rasterization driver defs

#include "r_shared.h"

//
// TODO: fine-tune this; it's based on providing some overage even if there
// is a 2k-wide scan, with subdivision every 8, for 256 spans of 12 bytes each
//
#define SCANBUFFERPAD		0x1000

#define R_SKY_SMASK	0x007F0000
#define R_SKY_TMASK	0x007F0000

#define DS_SPAN_LIST_END	-128

#define SURFCACHE_SIZE_AT_320X200	600*1024

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct sspan_s
{
	int				u, v, count;
} sspan_t;

typedef struct surfcache_s
{
	struct surfcache_s* next;
	struct surfcache_s** owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s* texture;	// checked for animating textures
	byte				data[4];	// width*height elements
} surfcache_t;

cvar_t	d_subdiv16;

float	scale_for_mip;

bool		d_roverwrapped;
surfcache_t* sc_rover;
surfcache_t* d_initial_rover;

float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;

fixed16_t	sadjust, tadjust;
fixed16_t	bbextents, bbextentt;


//void D_DrawSpans8(espan_t* pspans);
//void D_DrawSpans16(espan_t* pspans);
//void D_DrawZSpans(espan_t* pspans);
//void Turbulent8(espan_t* pspan);
void D_SpriteDrawSpans(sspan_t* pspan);

//void D_DrawSkyScans8(espan_t* pspan);
//void D_DrawSkyScans16(espan_t* pspan);

void R_ShowSubDiv(void);
// void (*prealspandrawer)(void);
surfcache_t* D_CacheSurface(msurface_t* surface, int miplevel);

int D_MipLevelForScale(float scale);

short* d_pzbuffer;
unsigned int d_zrowbytes, d_zwidth;

int* d_pscantable;
//int	d_scantable[MAXHEIGHT];

//int	d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

//int	d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift;

pixel_t* d_viewbuffer;

//short* zspantable[MAXHEIGHT];

int		d_minmip;
float	d_scalemip[3];

//void (*d_drawspans) (espan_t* pspan);

