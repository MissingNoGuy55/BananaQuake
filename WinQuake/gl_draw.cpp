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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "gl_vidnt.h"
#include "gl_draw.h"
#include <utility>

#define GL_COLOR_INDEX8_EXT     0x80E5

extern unsigned char d_15to8table[65536];

cvar_t		gl_nobind = {"gl_nobind", "0"};
cvar_t		gl_max_size = {"gl_max_size", "1024"};
cvar_t		gl_picmip = {"gl_picmip", "0"};
static GLint	gl_hardware_maxsize;

byte		*draw_chars;				// 8*8 graphic characters
CQuakePic			*draw_disc;
CQuakePic			*draw_backtile;

CGLTexture*			translate_texture;
CGLTexture*			char_texture;

byte		conback_buffer[sizeof(CQuakePic)];
CQuakePic	*conback = (CQuakePic*)&conback_buffer;

int		gl_lightmap_format = 4;
int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;


int		texels;

CGLTexture CGLRenderer::gltextures[MAX_GLTEXTURES];
int			numgltextures;

unsigned int d_8to24table_fbright[256];
unsigned int d_8to24table_fbright_fence[256];
unsigned int d_8to24table_nobright[256];
unsigned int d_8to24table_nobright_fence[256];
unsigned int d_8to24table_conchars[256];
unsigned int d_8to24table_shirt[256];
unsigned int d_8to24table_pants[256];

bool gl_texture_NPOT = false;

CGLTexture* CGLRenderer::char_texture = NULL;
CGLTexture* CGLRenderer::translate_texture = NULL;

CGLRenderer::CGLRenderer()
{
	active_lightmaps = 0;

	memset(allocated, 0, sizeof(allocated));
	memset(blocklights, 0, sizeof(blocklights));
	memset(lightmaps, 0, sizeof(lightmaps));
	memset(lightmap_modified, 0, sizeof(lightmap_modified));
	memset(lightmap_polys, 0, sizeof(lightmap_polys));
	memset(lightmap_rectchange, 0, sizeof(lightmap_rectchange));

	skytexturenum = 0;
	lightmap_bytes = 0;

	lightmap_textures = NULL;
	translate_texture = NULL;
	char_texture = NULL;
}

CGLRenderer::CGLRenderer(const CGLRenderer& src)
{
	active_lightmaps = 0;

	memset(allocated, 0, sizeof(allocated));
	memset(blocklights, 0, sizeof(blocklights));
	memset(lightmaps, 0, sizeof(lightmaps));
	memset(lightmap_modified, 0, sizeof(lightmap_modified));
	memset(lightmap_polys, 0, sizeof(lightmap_polys));
	memset(lightmap_rectchange, 0, sizeof(lightmap_rectchange));

	skytexturenum = 0;
	lightmap_bytes = 0;

	lightmap_textures = NULL;
	translate_texture = NULL;
	char_texture = NULL;
}

CGLRenderer::~CGLRenderer()
{
	active_lightmaps = 0;

	memset(allocated, 0, sizeof(allocated));
	memset(blocklights, 0, sizeof(blocklights));
	memset(lightmaps, 0, sizeof(lightmaps));
	memset(lightmap_modified, 0, sizeof(lightmap_modified));
	memset(lightmap_polys, 0, sizeof(lightmap_polys));
	memset(lightmap_rectchange, 0, sizeof(lightmap_rectchange));

	skytexturenum = 0;
	lightmap_bytes = 0;

	lightmap_textures = NULL;
	translate_texture = NULL;
	char_texture = NULL;
}

unsigned* CGLRenderer::GL_8to32(byte* in, int pixels, unsigned int* usepal)
{
	int i;
	unsigned* out, * data;

	out = data = g_MemCache->Hunk_Alloc<unsigned>(pixels * 4);

	for (i = 0; i < pixels; i++)
		*out++ = usepal[*in++];

	return data;
}

void CGLRenderer::GL_Bind (CGLTexture* tex)
{
	if (gl_nobind.value)
		tex = char_texture;
	if (currenttexture == tex)
		return;
	currenttexture = tex;

	glBindTexture(GL_TEXTURE_2D, (GLuint)tex->texnum);

	//#ifdef _WIN32
//	bindTexFunc (GL_TEXTURE_2D, texnum);
//#else
//	glBindTexture(GL_TEXTURE_2D, texnum);
//#endif
}

CGLTexture* CGLRenderer::GL_NewTexture()
{
	CGLTexture* glt;

	glt = &gltextures[numgltextures];
	numgltextures++;

	glGenTextures(1, &glt->texnum);

	return glt;
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		2
// #define	BLOCK_WIDTH		256
// #define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
bool		scrap_dirty;
CGLTexture* scrap_textures[MAX_SCRAPS];
int			scrap_texnum;

// returns a texture and the position inside it
int CGLRenderer::Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		bestx;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("Scrap_AllocBlock: full");
	return -1;
}

int	scrap_uploads;

void CGLRenderer::Scrap_Upload (void)
{
	char	name[8];
	int		i;

	scrap_uploads++;

	for (i=0 ; i<MAX_SCRAPS ; i++) {
		sprintf(name, "scrap%i", i);
		scrap_textures[i] = GL_LoadTexture(name, BLOCK_WIDTH, BLOCK_HEIGHT, scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
		/*g_GLRenderer->GL_Bind(scrap_textures[i]);
		GL_Upload8 (scrap_texels[i], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);*/
	}
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH] = {};
	CQuakePic		pic = {};
	byte		padding[32] = {};	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

int		pic_texels;
int		pic_count;

/*
================
GL_Pad -- return smallest power of two greater than or equal to s
================
*/
int CGLRenderer::GL_Pad(int s)
{
	int i;
	for (i = 1; i < s; i <<= 1)
		;
	return i;
}

/*
===============
GL_SafeTextureSize -- return a size with hardware and user prefs in mind
===============
*/
int CGLRenderer::GL_SafeTextureSize(int s)
{
	if (!gl_texture_NPOT)
		s = GL_Pad(s);
	if ((int)gl_max_size.value > 0)
		s = q_min(GL_Pad((int)gl_max_size.value), s);
	s = q_min(gl_hardware_maxsize, s);
	return s;
}

/*
================
GL_PadConditional -- only pad if a texture of that size would be padded. (used for tex coords)
================
*/
int CGLRenderer::GL_PadConditional(int s)
{
	if (s < GL_SafeTextureSize(s))
		return GL_Pad(s);
	else
		return s;
}

CQuakePic* CGLRenderer::Draw_PicFromWad(const char* name)
{
	CQuakePic* p = NULL;
	COpenGLPic	gl;
	int offset = 0; //Missi -- copied from QuakeSpasm (5/28/2022)
	int off = 0;

	p = static_cast<CQuakePic*>(W_GetLumpName(name));

	if (!p) return NULL; //Missi -- copied from QuakeSpasm (5/28/2022)


	//p->data.Init();
	// 
	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock(p->width, p->height, &x, &y);
		scrap_dirty = true;
		k = 0;
		for (i = 0; i < p->height; i++)
		{
			for (j = 0; j < p->width; j++, k++)
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];
		}
		gl.tex = scrap_textures[texnum]; //Missi -- copied from QuakeSpasm (5/28/2022) -- changed to an array
		//Missi -- copied from QuakeSpasm (5/28/2022) -- no longer go from 0.01 to 0.99
		gl.sl = x / (float)BLOCK_WIDTH;	// FIXME: Missi -- broken here! (6/14/2022)
		gl.sh = (x + p->width) / (float)BLOCK_WIDTH;
		gl.tl = y / (float)BLOCK_WIDTH;
		gl.th = (y + p->height) / (float)BLOCK_WIDTH;
	}
	else
	{
		//char texturename[64]; //Missi -- copied from QuakeSpasm (5/28/2022)
		//snprintf(texturename, sizeof(texturename), "%s:%s", texturename, name); //Missi -- copied from QuakeSpasm (5/28/2022)

		offset = (size_t)p - (size_t)wad_base + sizeof(size_t) * 2; //Missi -- copied from QuakeSpasm (5/28/2022)

		gl.tex = GL_LoadTexture(name, p->width, p->height, p->data, TEXPREF_ALPHA); //Missi -- copied from QuakeSpasm (5/28/2022) -- TexMgr
		gl.sl = 0;
		gl.sh = (float)p->width / (float)GL_PadConditional(p->width); //Missi -- copied from QuakeSpasm (5/28/2022)
		gl.tl = 0;
		gl.th = (float)p->height / (float)GL_PadConditional(p->height); //Missi -- copied from QuakeSpasm (5/28/2022)
	}

	memcpy(p->data, &gl, sizeof(COpenGLPic));

	return p;
}


/*
================
Draw_CachePic
================
*/
CQuakePic* CGLRenderer::Draw_CachePic (const char *path)
{
	cachepic_t	*pic = NULL;
	int			i = 0;
	CQuakePic		*qpic = NULL;
	byte			*qpictemp = NULL;
	byte			*qpicdata = NULL;
	COpenGLPic		gl;
	size_t			qpictemp_len = 0;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	Q_strcpy (pic->name, path);

//
// load the pic from disk
//
	qpic = COM_LoadTempFile<CQuakePic> (path);
	if (!qpic)
	{
		Sys_Error ("Draw_CachePic: failed to load %s", path);
		return NULL;
	}

	SwapPic (qpic);

	pic->pic.width = qpic->width;
	pic->pic.height = qpic->height;

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
#ifndef WIN64
		memcpy (menuplyr_pixels, &qpic->data, qpic->width * qpic->height);
#else
		memcpy(menuplyr_pixels, &qpic->data, (long long)qpic->width * (long long)qpic->height);
#endif
	/*qpic->width = qpicbuf->width;
	qpic->height = qpicbuf->height;*/

	gl.tex = GL_LoadTexture(path, qpic->width, qpic->height, qpic->data, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP); // (COpenGLPic*)pic->pic->data;
	gl.sl = 0;
	gl.sh = 1;
	gl.tl = 0;
	gl.th = 1;

	memcpy(pic->pic.data, &gl, sizeof(COpenGLPic));

 	return &pic->pic;
}


void CGLRenderer::Draw_CharToConback (int num, byte *dest)
{
#ifndef WIN64
	int		row, col;
#else
	long long	row, col;
#endif
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_TextureMode_f
===============
*/
void CGLRenderer::Draw_TextureMode_f (void)
{
	int		i;
	CGLTexture*	glt;

	if (Cmd_Argc() == 1)
	{
		for (i=0 ; i< 6 ; i++)
			if (gl_filter_min == modes[i].minimize)
			{
				Con_Printf ("%s\n", modes[i].name);
				return;
			}
		Con_Printf ("current filter is unknown???\n");
		return;
	}

	for (i=0 ; i< 6 ; i++)
	{
		if (!Q_strcasecmp (modes[i].name, Cmd_Argv(1) ) )
			break;
	}
	if (i == 6)
	{
		Con_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures; i<numgltextures ; i++)
	{
		if (glt->mipmap)
		{
			g_GLRenderer->GL_Bind (glt);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

#define TRANSFER_DATA(a, b) 

/*
===============
Draw_Init
===============
*/
void CGLRenderer::Draw_Init (void)
{
	int		i = 0;
	CQuakePic* cb =	NULL;
	byte	*dest = NULL, *src = NULL;
	int		x = 0, y = 0;
	char	ver[40] = "";
	COpenGLPic gl;
	int		start = 0;
	byte	*ncdata = NULL;
	int		f = 0, fstep = 0;
	size_t	s = 0;

	//gltexturevector.SetCount(0);

	Cvar_RegisterVariable (&gl_nobind);
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);

	// 3dfx can only handle 256 wide textures
	if (!Q_strncasecmp ((char *)gl_renderer, "3dfx",4) ||
		strstr((char *)gl_renderer, "Glide"))
		Cvar_Set ("gl_max_size", "256");

	Cmd_AddCommand ("gl_texturemode", &CGLRenderer::Draw_TextureMode_f);

	draw_chars = static_cast<byte*>(W_GetLumpName ("conchars"));
	
	// now turn them into textures
	char_texture = GL_LoadTexture ("charset", 128, 128, draw_chars, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);

	start = g_MemCache->Hunk_LowMark();

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	conback = Draw_CachePic("gfx/conback.lmp");
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	/*gl.tex = GL_LoadTexture ("conback", conback->width, conback->height, cb->data, TEXPREF_NOPICMIP | TEXPREF_ALPHA);
	gl.sl = 0;
	gl.sh = 1;
	gl.tl = 0;
	gl.th = 1;*/

	// free loaded console
	g_MemCache->Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	translate_texture = g_MemCache->Hunk_AllocName<CGLTexture>(sizeof(CGLTexture), "dummy");
	translate_texture->texnum = texture_extension_number++; // &gltextures[texture_extension_number++];

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	memset(scrap_allocated, 0, sizeof(scrap_allocated));
	memset(scrap_texels, 255, sizeof(scrap_texels));

	Scrap_Upload();

	//
	// get the other pics we need
	//
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void CGLRenderer::Draw_Character (int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;	
	int				row, col;
	float			frow, fcol, size;

	if (num == 32)
		return;		// space

	num &= 255;
	
	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	g_GLRenderer->GL_Bind (char_texture);

	glBegin (GL_QUADS);
	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + size, frow + size);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + size);
	glVertex2f (x, y+8);
	glEnd ();
}

/*
================
Draw_String
================
*/
void CGLRenderer::Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void CGLRenderer::Draw_DebugChar (char num)
{
}

/*
=============
Draw_AlphaPic
=============
*/
void CGLRenderer::Draw_AlphaPic (int x, int y, CQuakePic *pic, float alpha)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u;
	COpenGLPic		*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (COpenGLPic*)&pic->data;
	glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glCullFace(GL_FRONT);
	glColor4f (1,1,1,alpha);
	g_GLRenderer->GL_Bind (gl->tex);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
	glColor4f (1,1,1,1);
	glEnable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void CGLRenderer::Draw_Pic (int x, int y, CQuakePic *pic)
{
	COpenGLPic* gl;

	if (scrap_dirty)
		Scrap_Upload();
	gl = (COpenGLPic*)pic->data;
	GL_Bind(gl->tex);
	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2f(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2f(x + pic->width, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2f(x + pic->width, y + pic->height);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2f(x, y + pic->height);
	glEnd();

}


/*
=============
Draw_TransPic
=============
*/
void CGLRenderer::Draw_TransPic (int x, int y, CQuakePic *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void CGLRenderer::Draw_TransPicTranslate (int x, int y, CQuakePic *pic, byte *translation)
{
	int				v, u, c;
	static unsigned		trans[64 * 64];
	unsigned	*dest;
	byte			*src;
	int				p;

	g_GLRenderer->GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColor3f (1,1,1);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (1, 0);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (1, 1);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (0, 1);
	glVertex2f (x, y+pic->height);
	glEnd ();
}


/*
================
Draw_ConsoleBackground

================
*/
void CGLRenderer::Draw_ConsoleBackground (int lines)
{
	int y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_Pic(0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void CGLRenderer::Draw_TileClear (int x, int y, int w, int h)
{
	COpenGLPic* gl;

	gl = (COpenGLPic*)draw_backtile->data;

	glColor3f (1,1,1);
	g_GLRenderer->GL_Bind(gl->tex); // *(int*)draw_backtile->data;
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0f, y/64.0f);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0f, y/64.0f);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0f, (y+h)/64.0f);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0f, (y+h)/64.0f );
	glVertex2f (x, y+h);
	glEnd ();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void CGLRenderer::Draw_Fill (int x, int y, int w, int h, int c)
{
	glDisable (GL_TEXTURE_2D);
	glColor3f (host->host_basepal[c*3]/255.0,
		host->host_basepal[c*3+1]/255.0,
		host->host_basepal[c*3+2]/255.0);

	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void CGLRenderer::Draw_FadeScreen (void)
{
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, 0.8);
	glBegin (GL_QUADS);

	glVertex2f (0,0);
	glVertex2f (vid.width, 0);
	glVertex2f (vid.width, vid.height);
	glVertex2f (0, vid.height);

	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void CGLRenderer::Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;
	glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, draw_disc);
	glDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void CGLRenderer::Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void CGLRenderer::GL_Set2D (void)
{
	glViewport (glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
//	glDisable (GL_ALPHA_TEST);

	glColor4f (1,1,1,1);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
CGLTexture* CGLRenderer::GL_FindTexture (const char *identifier)
{
	int		i;
	CGLTexture* glt;

	for (i=0, glt=gltextures; i<numgltextures ; i++)
	{
		if (!strcmp (identifier, glt->identifier))
			return &gltextures[i];
	}

	return NULL;
}

/*
================
TexMgr_MipMapW
================
*/
unsigned* CGLRenderer::GL_MipMapW(unsigned* data, int width, int height)
{
	int	i, size;
	byte* out, * in;

	out = in = (byte*)data;
	size = (width * height) >> 1;

	for (i = 0; i < size; i++, out += 4, in += 8)
	{
		out[0] = (in[0] + in[4]) >> 1;
		out[1] = (in[1] + in[5]) >> 1;
		out[2] = (in[2] + in[6]) >> 1;
		out[3] = (in[3] + in[7]) >> 1;
	}

	return data;
}

/*
================
TexMgr_MipMapH
================
*/
unsigned* CGLRenderer::GL_MipMapH(unsigned* data, int width, int height)
{
	int	i, j;
	byte* out, * in;

	out = in = (byte*)data;
	height >>= 1;
	width <<= 2;

	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 4, out += 4, in += 4)
		{
			out[0] = (in[0] + in[width + 0]) >> 1;
			out[1] = (in[1] + in[width + 1]) >> 1;
			out[2] = (in[2] + in[width + 2]) >> 1;
			out[3] = (in[3] + in[width + 3]) >> 1;
		}
	}

	return data;
}

/*
================
GL_ResampleTexture
================
*/
#ifndef WIN64
unsigned* CGLRenderer::GL_ResampleTexture (unsigned *in, int inwidth, int inheight, bool alpha)
#else
unsigned* CGLRenderer::GL_ResampleTexture (unsigned long long *in, int inwidth, int inheight, bool alpha)
#endif
{
	byte* nwpx, * nepx, * swpx, * sepx, * dest;
	unsigned xfrac, yfrac, x, y, modx, mody, imodx, imody, injump, outjump;
	unsigned* out;
	int i, j, outwidth, outheight;

	if (inwidth == GL_Pad(inwidth) && inheight == GL_Pad(inheight))
		return in;

	outwidth = GL_Pad(inwidth);
	outheight = GL_Pad(inheight);
	out = (unsigned*)g_MemCache->Hunk_Alloc<unsigned>(outwidth * outheight * 4);

	xfrac = ((inwidth - 1) << 16) / (outwidth - 1);
	yfrac = ((inheight - 1) << 16) / (outheight - 1);
	y = outjump = 0;

	for (i = 0; i < outheight; i++)
	{
		mody = (y >> 8) & 0xFF;
		imody = 256 - mody;
		injump = (y >> 16) * inwidth;
		x = 0;

		for (j = 0; j < outwidth; j++)
		{
			modx = (x >> 8) & 0xFF;
			imodx = 256 - modx;

			nwpx = (byte*)(in + (x >> 16) + injump);
			nepx = nwpx + 4;
			swpx = nwpx + inwidth * 4;
			sepx = swpx + 4;

			dest = (byte*)(out + outjump + j);

			dest[0] = (nwpx[0] * imodx * imody + nepx[0] * modx * imody + swpx[0] * imodx * mody + sepx[0] * modx * mody) >> 16;
			dest[1] = (nwpx[1] * imodx * imody + nepx[1] * modx * imody + swpx[1] * imodx * mody + sepx[1] * modx * mody) >> 16;
			dest[2] = (nwpx[2] * imodx * imody + nepx[2] * modx * imody + swpx[2] * imodx * mody + sepx[2] * modx * mody) >> 16;
			if (alpha)
				dest[3] = (nwpx[3] * imodx * imody + nepx[3] * modx * imody + swpx[3] * imodx * mody + sepx[3] * modx * mody) >> 16;
			else
				dest[3] = 255;

			x += xfrac;
		}
		outjump += outwidth;
		y += yfrac;
	}

	return out;
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void CGLRenderer::GL_Resample8BitTexture (unsigned char *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
#ifndef WIN64
	int		i, j;
#else
	long long	i, j;
#endif
	unsigned	char *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}


/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void CGLRenderer::GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void CGLRenderer::GL_MipMap8Bit (byte *in, int width, int height)
{
	int		i, j;
	unsigned short     r,g,b;
	byte	*out, *at1, *at2, *at3, *at4;

//	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=2, out+=1, in+=2)
		{
			at1 = (byte *) (d_8to24table + in[0]);
			at2 = (byte *) (d_8to24table + in[1]);
			at3 = (byte *) (d_8to24table + in[width+0]);
			at4 = (byte *) (d_8to24table + in[width+1]);

 			r = (at1[0]+at2[0]+at3[0]+at4[0]); r>>=5;
 			g = (at1[1]+at2[1]+at3[1]+at4[1]); g>>=5;
 			b = (at1[2]+at2[2]+at3[2]+at4[2]); b>>=5;

			out[0] = d_15to8table[(r<<0) + (g<<5) + (b<<10)];
		}
	}
}

/*
===============
GL_Upload32
===============
*/
#ifndef WIN64
void CGLRenderer::GL_Upload32 (CGLTexture* tex, unsigned* data)
#else
void CGLRenderer::GL_Upload32 (CGLTexture* tex, unsigned long long* data)
#endif
{
	int	internalformat, miplevel, mipwidth, mipheight, picmip;

	// upload
	GL_Bind(tex);
	internalformat = (tex->flags & TEXPREF_ALPHA) ? gl_alpha_format : gl_solid_format;
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	if (tex->flags & TEXPREF_MIPMAP && !(tex->flags & TEXPREF_WARPIMAGE)) // warp image mipmaps are generated later
	{
		mipwidth = tex->width;
		mipheight = tex->height;

		for (miplevel = 1; mipwidth > 1 || mipheight > 1; miplevel++)
		{
			if (mipwidth > 1)
			{
				GL_MipMapW(data, mipwidth, mipheight);
				mipwidth >>= 1;
			}
			if (mipheight > 1)
			{
				GL_MipMapH(data, mipwidth, mipheight);
				mipheight >>= 1;
			}
			glTexImage2D(GL_TEXTURE_2D, miplevel, internalformat, mipwidth, mipheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
done: ;

	if (tex->mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

/*
================
GL_FreeTexture -- Missi (7/5/2022)

Loads an OpenGL texture via string ID or byte data. Can take an empty string or NULL if you don't need a name.
================
*/

/*
================
GL_LoadTexture -- Missi: revised (3/18/2022)

Loads an OpenGL texture via string ID or byte data. Can take an empty string or NULL if you don't need a name.
================
*/
CGLTexture* CGLRenderer::GL_LoadTexture (const char *identifier, int width, int height, byte *data, int flags)
{
	int			i = 0, p = 0, s = 0, l = 0, m = 0; // -- Missi
	CGLTexture* glt = NULL;
	COpenGLPic*	gl;
	CQuakePic* qpic = NULL;
	byte* out = data;
	int			CRCBlock1 = 0, CRCBlock2 = 0;
	byte padbyte;
	unsigned int* usepal;
	char id[64];
	int len = 0;

// Missi: the below two lines used to be an else statement in vanilla GLQuake... with horrible results (3/22/2022)

	if ((glt = GL_FindTexture(identifier)) && (glt->flags & TEXPREF_OVERWRITE))
		return glt;
	else
		glt = GL_NewTexture();

	glt->pic.width = width;
	glt->pic.height = height;

	Q_strncpy(glt->identifier, identifier, sizeof(glt->identifier));	// Missi: this was Q_strcpy at one point. Caused heap corruption. (4/11/2022)
	glt->texnum = texture_extension_number;
	glt->width = width;
	glt->height = height;
	glt->flags = flags;

	// choose palette and padbyte
	if (glt->flags & TEXPREF_FULLBRIGHT)
	{
		if (glt->flags & TEXPREF_ALPHA)
			usepal = d_8to24table_fbright_fence;
		else
			usepal = d_8to24table_fbright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_NOBRIGHT) // && gl_fullbrights.value)
	{
		if (glt->flags & TEXPREF_ALPHA)
			usepal = d_8to24table_nobright_fence;
		else
			usepal = d_8to24table_nobright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_CONCHARS)
	{
		usepal = d_8to24table_conchars;
		padbyte = 0;
	}
	else
	{
		usepal = d_8to24table;
		padbyte = 255;
	}

	//g_GLRenderer->GL_Bind(glt);

	data = (byte*)GL_8to32(data, glt->width * glt->height, usepal);

#ifndef WIN64
	GL_Upload32(glt, (unsigned*)data);
#else
	GL_Upload32(glt, (unsigned long long*)data);
#endif

	texture_extension_number++;

	return glt;

}

/*
================
GL_LoadPicTexture
================
*/
CGLTexture* CGLRenderer::GL_LoadPicTexture (CQuakePic* pic)
{
	return GL_LoadTexture("", pic->width, pic->height, pic->data, TEXPREF_ALPHA);
}

/****************************************/

static GLenum oldtarget = TEXTURE0_SGIS;

void CGLRenderer::GL_SelectTexture (GLenum target)
{
	if (!gl_mtexable)
		return;
	qglSelectTextureSGIS(target);
	if (target == oldtarget) 
		return;
	cnttextures[oldtarget-TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target-TEXTURE0_SGIS];
	oldtarget = target;
}

CGLTexture::CGLTexture() : height(0), width(0), mipmap(false), texnum(0), checksum(0), next(NULL)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::CGLTexture(CQuakePic qpic, float des_sl, float des_tl, float des_sh, float des_th) : height(qpic.height), width(qpic.width), mipmap(false), texnum(0), checksum(0), next(NULL)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::CGLTexture(const CGLTexture& obj) : height(0), width(0), mipmap(false), texnum(0), checksum(0), next(NULL)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::~CGLTexture()
{
	memset(&pic.data, 0, sizeof(CQuakePic));
}
