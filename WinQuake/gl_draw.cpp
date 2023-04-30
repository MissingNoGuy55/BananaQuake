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

// gl_draw.cpp -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

// Missi: already defined in GL.h in Windows and SDL_OpenGL_glext.h (4/29/2023)
//#define GL_COLOR_INDEX8_EXT     0x80E5

// unsigned char d_15to8table[65536];

unsigned int d_8to24table_fbright[256];
unsigned int d_8to24table_fbright_fence[256];
unsigned int d_8to24table_nobright[256];
unsigned int d_8to24table_nobright_fence[256];
unsigned int d_8to24table_conchars[256];
unsigned int d_8to24table_shirt[256];
unsigned int d_8to24table_pants[256];

bool gl_texture_NPOT = false;

static cvar_t		gl_nobind = {"gl_nobind", "0"};
static cvar_t		gl_picmip = {"gl_picmip", "0"};
static cvar_t		gl_texturemode = {"gl_texturemode", ""};

cvar_t		gl_max_size = {"gl_max_size", "1024"};
cvar_t		gl_fullbrights = {"gl_fullbrights", "0"};
GLint		gl_hardware_maxsize;

//byte				*draw_chars;				// 8*8 graphic characters
//CQuakePic			*draw_disc;
//CQuakePic			*draw_backtile;

//CGLTexture*		translate_texture;
//CGLTexture*		char_texture;

static byte	conback_buffer[sizeof(CQuakePic)];
CQuakePic	*conback = (CQuakePic*)&conback_buffer;

CGLTexture	CGLRenderer::gltextures[MAX_GLTEXTURES];
CGLTexture* CGLRenderer::free_gltextures;
CGLTexture* CGLRenderer::active_gltextures;
CGLTexture* CGLRenderer::lightmap_textures;
byte		CGLRenderer::lightmaps[4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];// [MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];

CGLTexture* CGLRenderer::char_texture;
CGLTexture* CGLRenderer::translate_texture;

int			CGLRenderer::allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];
int			CGLRenderer::lightmap_count;
glpoly_t*	CGLRenderer::lightmap_polys[MAX_LIGHTMAPS];
bool		CGLRenderer::lightmap_modified[MAX_LIGHTMAPS];
glRect_t	CGLRenderer::lightmap_rectchange[MAX_LIGHTMAPS];

int			CGLRenderer::gl_filter_min;
int			CGLRenderer::gl_filter_max;

CGLRenderer::CGLRenderer()
{
	active_lightmaps = 0;

	memset(d_8to24table_fbright, 0, sizeof(d_8to24table_fbright));
	memset(d_8to24table_fbright_fence, 0, sizeof(d_8to24table_fbright_fence));
	memset(d_8to24table_nobright, 0, sizeof(d_8to24table_nobright));
	memset(d_8to24table_nobright_fence, 0, sizeof(d_8to24table_nobright_fence));
	memset(d_8to24table_conchars, 0, sizeof(d_8to24table_conchars));
	memset(d_8to24table_shirt, 0, sizeof(d_8to24table_shirt));
	memset(d_8to24table_pants, 0, sizeof(d_8to24table_pants));

	memset(allocated, 0, sizeof(allocated));
	memset(blocklights, 0, sizeof(blocklights));
	memset(lightmaps, 0, sizeof(lightmaps));
	memset(lightmap_modified, 0, sizeof(lightmap_modified));
	memset(lightmap_polys, 0, sizeof(lightmap_polys));
	memset(lightmap_rectchange, 0, sizeof(lightmap_rectchange));

	skytexturenum = 0;
	lightmap_bytes = 0;
	lightmap_count = 0;
	last_lightmap_allocated = 0;

	draw_chars = NULL;
	draw_disc = NULL;
	draw_backtile = NULL;

	skychain = NULL;
	waterchain = NULL;

	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
	gl_filter_max = GL_LINEAR;

	lightmap_textures = NULL;

	free_gltextures = g_MemCache->Hunk_AllocName<CGLTexture>(sizeof(CGLTexture) * MAX_GLTEXTURES, "gltextures");
	active_gltextures = NULL;
	int i;

	for (i = 0; i < MAX_GLTEXTURES - 1; i++)
		free_gltextures[i].next = &free_gltextures[i + 1];
	free_gltextures[i].next = NULL;

	numgltextures = 0;
	texels = 0;

	gl_lightmap_format = 4;
	gl_solid_format = 3;
	gl_alpha_format = 4;

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

	skytexturenum = 0;
	lightmap_bytes = 0;
	lightmap_count = 0;
	last_lightmap_allocated = 0;

	lightmap_textures = NULL;
	translate_texture = NULL;
	char_texture = NULL;

	free_gltextures = NULL;
	active_gltextures = NULL;

	numgltextures = 0;
}

typedef struct
{
	int	magfilter;
	int	minfilter;
	const char* name;
} glmode_t;

static glmode_t glmodes[] = {
	{GL_NEAREST, GL_NEAREST,		"GL_NEAREST"},
	{GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST,	"GL_NEAREST_MIPMAP_NEAREST"},
	{GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR,	"GL_NEAREST_MIPMAP_LINEAR"},
	{GL_LINEAR,  GL_LINEAR,			"GL_LINEAR"},
	{GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,	"GL_LINEAR_MIPMAP_NEAREST"},
	{GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR,	"GL_LINEAR_MIPMAP_LINEAR"},
};

#define NUM_GLMODES (int)(sizeof(glmodes)/sizeof(glmodes[0]))
static int glmode_idx = NUM_GLMODES - 1;

unsigned* CGLRenderer::GL_8to32(byte* in, int pixels, unsigned int* usepal)
{
	int i;
	unsigned* out, * data;

	out = data = g_MemCache->Hunk_Alloc<unsigned>(pixels * 4);

	for (i = 0; i < pixels; i++)
		*out++ = usepal[*in++];

	return data;
}

/*
===============
CGLRenderer::GL_AlphaEdgeFix

eliminate pink edges on sprites, etc.
operates in place on 32bit data
===============
*/
void CGLRenderer::GL_AlphaEdgeFix(byte* data, int width, int height)
{
	int	i, j, n = 0, b, c[3] = { 0,0,0 },
		lastrow, thisrow, nextrow,
		lastpix, thispix, nextpix;
	byte* dest = data;

	for (i = 0; i < height; i++)
	{
		lastrow = width * 4 * ((i == 0) ? height - 1 : i - 1);
		thisrow = width * 4 * i;
		nextrow = width * 4 * ((i == height - 1) ? 0 : i + 1);

		for (j = 0; j < width; j++, dest += 4)
		{
			if (dest[3]) //not transparent
				continue;

			lastpix = 4 * ((j == 0) ? width - 1 : j - 1);
			thispix = 4 * j;
			nextpix = 4 * ((j == width - 1) ? 0 : j + 1);

			b = lastrow + lastpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = thisrow + lastpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = nextrow + lastpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = lastrow + thispix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = nextrow + thispix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = lastrow + nextpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = thisrow + nextpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }
			b = nextrow + nextpix; if (data[b + 3]) { c[0] += data[b]; c[1] += data[b + 1]; c[2] += data[b + 2]; n++; }

			//average all non-transparent neighbors
			if (n)
			{
				dest[0] = (byte)(c[0] / n);
				dest[1] = (byte)(c[1] / n);
				dest[2] = (byte)(c[2] / n);

				n = c[0] = c[1] = c[2] = 0;
			}
		}
	}
}

void CGLRenderer::GL_Bind (CGLTexture* tex)
{
	if (gl_nobind.value)
		tex = char_texture;
	if (currenttexture == tex)
		return;
	currenttexture = tex;

	glBindTexture(GL_TEXTURE_2D, (GLuint)tex->texnum);
}

CGLTexture* CGLRenderer::GL_NewTexture()
{
	CGLTexture* glt = NULL;

	if (numgltextures == MAX_GLTEXTURES)
		Sys_Error("numgltextures == MAX_GLTEXTURES\n");

	glt = free_gltextures;
	free_gltextures = glt->next;
	glt->next = active_gltextures;
	active_gltextures = glt;

	glGenTextures(1, &glt->texnum);
	numgltextures++;
	return glt;
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

static constexpr short MAX_SCRAPS = 2;

//#define	MAX_SCRAPS		2
//#define	BLOCK_WIDTH		256
//#define	BLOCK_HEIGHT	256

static int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
static byte			scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
static bool			scrap_dirty;
static CGLTexture*	scrap_textures[MAX_SCRAPS];
static int			scrap_texnum;

// returns a texture and the position inside it
int CGLRenderer::Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
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

static int	scrap_uploads;

void CGLRenderer::Scrap_Upload (void)
{
	char	name[8];
	int		i;

	scrap_uploads++;

	for (i=0 ; i<MAX_SCRAPS ; i++)
	{
		snprintf(name, sizeof(name), "scrap%i", i);
		scrap_textures[i] = GL_LoadTexture(NULL, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_INDEXED, scrap_texels[i],
			(src_offset_t)scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
		/*g_GLRenderer->GL_Bind(scrap_textures[i]);
		GL_Upload8 (scrap_texels[i], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);*/
	}
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	CQuakePic		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		128
static cachepic_t	menu_cachepics[MAX_CACHED_PICS];
static int			menu_numcachepics;

static byte		menuplyr_pixels[4096];

/*
================
CGLRenderer::GL_Pad -- return smallest power of two greater than or equal to s
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
CGLRenderer::GL_SafeTextureSize -- return a size with hardware and user prefs in mind
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
CGLRenderer::GL_PadConditional -- only pad if a texture of that size would be padded. (used for tex coords)
================
*/
int CGLRenderer::GL_PadConditional(int s)
{
	if (s < GL_SafeTextureSize(s))
		return GL_Pad(s);
	else
		return s;
}

/*
===============
CGLRenderer::Draw_PicFromWad
===============
*/
CQuakePic* CGLRenderer::Draw_PicFromWad(const char* name)
{
	CQuakePic* p = NULL;
	qpicbuf_t* pbuf = NULL;
	COpenGLPic	gl;
	uintptr_t offset;

	pbuf = W_GetLumpName<qpicbuf_t>(name);
	
	if (!pbuf) return NULL; //Missi -- copied from QuakeSpasm (5/28/2022)

	p = g_MemCache->Hunk_AllocName<CQuakePic>(pbuf->width * pbuf->height, name);
	p->width = pbuf->width;
	p->height = pbuf->height;

	// load little ones into the scrap
	if (p->width < 64 && pbuf->height < 64)
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
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = pbuf->data[k];
		}
		gl.tex = scrap_textures[texnum]; //Missi -- copied from QuakeSpasm (5/28/2022) -- changed to an array
		//Missi -- copied from QuakeSpasm (5/28/2022) -- no longer go from 0.01 to 0.99
		gl.sl = x / (float)BLOCK_WIDTH;
		gl.sh = (x + p->width) / (float)BLOCK_WIDTH;
		gl.tl = y / (float)BLOCK_WIDTH;
		gl.th = (y + p->height) / (float)BLOCK_WIDTH;
	}
	else
	{

		offset = (uintptr_t)p - (uintptr_t)wad_base + sizeof(int) * 2; //johnfitz

		gl.tex = GL_LoadTexture(NULL, name, p->width, p->height, SRC_INDEXED, pbuf->data, offset, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP); //Missi -- copied from QuakeSpasm (5/28/2022) -- TexMgr
		gl.sl = 0;
		gl.sh = (float)p->width / (float)GL_PadConditional(p->width); //Missi -- copied from QuakeSpasm (5/28/2022)
		gl.tl = 0;
		gl.th = (float)p->height / (float)GL_PadConditional(p->height); //Missi -- copied from QuakeSpasm (5/28/2022)
	}

	//memset(&p->datavec, 0, sizeof(p->datavec));

	p->datavec.AddMultipleToTail(sizeof(COpenGLPic), (byte*)&gl);
    
	return p;
}


/*
================
CGLRenderer::Draw_CachePic
================
*/
CQuakePic* CGLRenderer::Draw_CachePic (const char *path)
{
	cachepic_t		*pic = NULL;
	int				i = 0;
	qpicbuf_t		*qpicbuf = NULL;
	COpenGLPic		gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");

	menu_numcachepics++;
	Q_strncpy (pic->name, path, sizeof(pic->name));

//
// load the pic from disk
//
	qpicbuf = COM_LoadTempFile<qpicbuf_t> (path, NULL);
	if (!qpicbuf)
	{
		Sys_Error ("Draw_CachePic: failed to load %s", path);
		return NULL;
	}

	SwapPic (qpicbuf);
	pic->pic.width = qpicbuf->width;
	pic->pic.height = qpicbuf->height;

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, pic->pic.datavec.Base(), qpicbuf->width * qpicbuf->height);

	gl.tex = GL_LoadTexture(NULL, path, qpicbuf->width, qpicbuf->height, SRC_INDEXED, qpicbuf->data, sizeof(int) * 2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP); // (COpenGLPic*)pic->pic->data;
	gl.sl = 0;
	gl.sh = 1;
	gl.tl = 0;
	gl.th = 1;

	pic->pic.datavec.AddMultipleToTail(sizeof(COpenGLPic), (byte*)&gl);
	//memcpy(pic->pic.data, &gl, sizeof(COpenGLPic));

 	return &pic->pic;
}

/*
===============
CGLRenderer::Draw_CharToConback
===============
*/
void CGLRenderer::Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
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

/*
===============
CGLRenderer::Draw_TextureMode_f
===============
*/
void CGLRenderer::Draw_TextureMode_f (void)
{
	int		i;
	CGLTexture	*glt = NULL;

	if (Cmd_Argc() == 1)
	{
		for (i=0 ; i< NUM_GLMODES; i++)
			if (gl_filter_min == glmodes[i].minfilter)
			{
				Con_Printf ("%s\n", glmodes[i].name);
				return;
			}
		Con_Printf ("current filter is unknown???\n");
		return;
	}

	for (i=0 ; i< NUM_GLMODES; i++)
	{
		if (!Q_strcasecmp (glmodes[i].name, Cmd_Argv(1) ) )
			break;
	}
	if (i == NUM_GLMODES)
	{
		Con_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = glmodes[i].minfilter;
	gl_filter_max = glmodes[i].magfilter;

	gl_texturemode.string = Cmd_Argv(1);

	// change all the existing mipmap texture objects
	for (i = 0; i < NUM_GLMODES; i++)
	{
		if (!Q_strcmp(glmodes[i].name, gl_texturemode.string))
		{
			if (glmode_idx != i)
			{
				glmode_idx = i;
				for (glt = active_gltextures; glt; glt = glt->next)
					GL_SetFilterModes(glt);
				Sbar_Changed(); //sbar graphics need to be redrawn with new filter mode
				//FIXME: warpimages need to be redrawn, too.
			}
			return;
		}
	}

	for (i = 0; i < NUM_GLMODES; i++)
	{
		if (!Q_strcasecmp(glmodes[i].name, gl_texturemode.string))
		{
			gl_texturemode.string = Cmd_Argv(1);
			return;
		}
	}

	i = atoi(gl_texturemode.string);
	if (i >= 1 && i <= NUM_GLMODES)
	{
		gl_texturemode.string = Cmd_Argv(1);
		return;
	}
}

/*
===============
CGLRenderer::Draw_Init
===============
*/
void CGLRenderer::Draw_Init (void)
{
	char	ver[40];
	int		start = 0;
	uintptr_t offset = 0;

	Cvar_RegisterVariable (&gl_nobind);
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);

	memset(ver, 0, sizeof(ver));

	Cmd_AddCommand ("gl_texturemode", &CGLRenderer::Draw_TextureMode_f);

	draw_chars = W_GetLumpName<byte>("conchars");
	offset = (uintptr_t)draw_chars - (uintptr_t)wad_base;

	// now turn them into textures
	char_texture = GL_LoadTexture (NULL, "charset", 128, 128, SRC_INDEXED, draw_chars, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);

	start = g_MemCache->Hunk_LowMark();

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	conback = Draw_CachePic("gfx/conback.lmp");
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

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
CGLRenderer::Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void CGLRenderer::Draw_Character (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	if (num == 32)
		return;		// space

	num &= 255;
	
	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625f;
	fcol = col*0.0625f;
	size = 0.0625f;

	GL_Bind (char_texture);

	glBegin (GL_QUADS);
	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow);
	glVertex2f (x + 8, y);
	glTexCoord2f (fcol + size, frow + size);
	glVertex2f (x + 8, y + 8);
	glTexCoord2f (fcol, frow + size);
	glVertex2f (x, y + 8);
	glEnd ();
}

/*
================
CGLRenderer::Draw_String
================
*/
void CGLRenderer::Draw_String (int x, int y, const char *str)
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
CGLRenderer::Draw_DebugChar

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
CGLRenderer::Draw_AlphaPic
=============
*/
void CGLRenderer::Draw_AlphaPic (int x, int y, CQuakePic *pic, float alpha)
{
	COpenGLPic		*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (COpenGLPic*)pic->datavec.Base();
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
CGLRenderer::Draw_Pic
=============
*/
void CGLRenderer::Draw_Pic (int x, int y, CQuakePic *pic)
{
	COpenGLPic* gl;

	if (scrap_dirty)
		Scrap_Upload();
	gl = (COpenGLPic*)pic->datavec.Base();
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
CGLRenderer::Draw_TransPic
=============
*/
void CGLRenderer::Draw_TransPic (int x, int y, CQuakePic *pic)
{
	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	Draw_Pic (x, y, pic);
}


/*
=============
CGLRenderer::Draw_TransPicTranslate

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
CGLRenderer::Draw_ConsoleBackground

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
CGLRenderer::Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void CGLRenderer::Draw_TileClear (int x, int y, int w, int h)
{
	COpenGLPic* gl;

	gl = (COpenGLPic*)draw_backtile->datavec.Base();

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
CGLRenderer::Draw_Fill

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
CGLRenderer::Draw_FadeScreen

================
*/
void CGLRenderer::Draw_FadeScreen (void)
{
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, 0.8f);
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
CGLRenderer::Draw_BeginDisc

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
CGLRenderer::Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void CGLRenderer::Draw_EndDisc (void)
{
}

/*
================
CGLRenderer::GL_Set2D

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
CGLRenderer::GL_FindTexture
================
*/
CGLTexture* CGLRenderer::GL_FindTexture (model_t* model, const char *identifier)
{
	CGLTexture* glt;

	if (identifier)
	{
		for (glt = active_gltextures; glt; glt = glt->next)
		{
			if (glt->owner == model && !strcmp(glt->identifier, identifier))
				return glt;
		}
	}

	return NULL;
}

void CGLRenderer::GL_FreeTextureForModel(model_t* mod)
{
	CGLTexture* glt, * next;

	for (glt = active_gltextures; glt; glt = next)
	{
		next = glt->next;
		if (glt && glt->owner == mod)
			GL_FreeTexture(glt);
	}
}

/*
================
CGLRenderer::GL_FreeTexture
================
*/
void CGLRenderer::GL_FreeTexture(CGLTexture* kill)
{
	CGLTexture* glt, *next;

	if (kill == NULL)
	{
		Con_Printf("GL_FreeTexture: NULL texture\n");
		return;
	}

	if (active_gltextures == kill)
	{
		active_gltextures = kill->next;
		kill->next = free_gltextures;
		free_gltextures = kill;

		GL_DeleteTexture(kill);
		numgltextures--;
		return;
	}

	for (glt = active_gltextures; glt; glt = glt->next)
	{
		next = glt->next;

		if (glt->next == kill)
		{
			glt->next = kill->next;
			kill->next = free_gltextures;
			free_gltextures = kill;

			GL_DeleteTexture(kill);
			numgltextures--;
			return;
		}
	}

	Con_Printf("GL_FreeTexture: not found\n");
}

/*
================
CGLRenderer::GL_DeleteTexture -- ericw

Wrapper around glDeleteTextures that also clears the given texture number
from our per-TMU cached texture binding table.
================
*/
void CGLRenderer::GL_DeleteTexture(CGLTexture* texture)
{
	glDeleteTextures(1, &texture->texnum);

	if (texture == &currenttexture[0]) memset(currenttexture, 0, 1);
	if (texture == &currenttexture[1]) memset(currenttexture, 0, 1);
	if (texture == &currenttexture[2]) memset(currenttexture, 0, 1);

	texture->texnum = 0;
}

/*
================
CGLRenderer::GL_MipMapW
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
CGLRenderer::GL_MipMapH
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
CGLRenderer::GL_ResampleTexture
================
*/
unsigned* CGLRenderer::GL_ResampleTexture (unsigned *in, int inwidth, int inheight, bool alpha)
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
CGLRenderer::GL_Resample8BitTexture -- JACK
================
*/
void CGLRenderer::GL_Resample8BitTexture (byte *in, int inwidth, int inheight, byte*out,  int outwidth, int outheight)
{
	int		i, j;
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
CGLRenderer::GL_MipMap

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
CGLRenderer::GL_MipMap8Bit

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

void CGLRenderer::GL_SetFilterModes(CGLTexture* glt)
{
	GL_Bind(glt);

	if (glt->flags & TEXPREF_NEAREST)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else if (glt->flags & TEXPREF_LINEAR)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	else if (glt->flags & TEXPREF_MIPMAP)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glmodes[glmode_idx].magfilter);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glmodes[glmode_idx].minfilter);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glmodes[glmode_idx].magfilter);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glmodes[glmode_idx].magfilter);
	}
}

/*
===============
CGLRenderer::GL_Upload8
===============
*/
void CGLRenderer::GL_Upload8(byte* data, int width, int height, bool mipmap, bool alpha)
{
	static	unsigned	trans[640 * 480];		// FIXME, temporary
	int			i, s;
	bool		noalpha;
	int			p;
	qpicbuf_t*	buf;
	CGLTexture	tex;

	buf = (qpicbuf_t*)data;
	tex.width = width;
	tex.height = height;

	s = width * height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (alpha)
	{
		noalpha = true;
		for (i = 0; i < s; i++)
		{
			p = data[i];
			if (p == 255)
				noalpha = false;
			trans[i] = d_8to24table[p];
		}

		if (alpha && noalpha)
			alpha = false;
	}
	else
	{
		if (s & 3)
			Sys_Error("GL_Upload8: s&3");
		for (i = 0; i < s; i += 4)
		{
			trans[i] = d_8to24table[data[i]];
			trans[i + 1] = d_8to24table[data[i + 1]];
			trans[i + 2] = d_8to24table[data[i + 2]];
			trans[i + 3] = d_8to24table[data[i + 3]];
		}
	}

	tex.pic.datavec.AddMultipleToTail(s, (byte*)trans);

	GL_Upload32(&tex, trans);
}

/*
===============
CGLRenderer::GL_Upload32
===============
*/
void CGLRenderer::GL_Upload32 (CGLTexture* tex, unsigned* data)
{
	int	internalformat, miplevel, mipwidth, mipheight, picmip;

	if (!gl_texture_NPOT)
	{
		// resample up
		data = GL_ResampleTexture(data, tex->width, tex->height, tex->flags & TEXPREF_ALPHA);
		tex->width = GL_Pad(tex->width);
		tex->height = GL_Pad(tex->height);
	}

	// upload
	GL_Bind(tex);
	internalformat = (tex->flags & TEXPREF_ALPHA) ? gl_alpha_format : gl_solid_format;
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// mipmap down
	picmip = (tex->flags & TEXPREF_NOPICMIP) ? 0 : q_max((int)gl_picmip.value, 0);
	mipwidth = GL_SafeTextureSize(tex->width >> picmip);
	mipheight = GL_SafeTextureSize(tex->height >> picmip);
	while ((int)tex->width > mipwidth)
	{
		GL_MipMapW(data, tex->width, tex->height);
		tex->width >>= 1;
		if (tex->flags & TEXPREF_ALPHA)
			GL_AlphaEdgeFix((byte*)data, tex->width, tex->height);
	}
	while ((int)tex->height > mipheight)
	{
		GL_MipMapH(data, tex->width, tex->height);
		tex->height >>= 1;
		if (tex->flags & TEXPREF_ALPHA)
			GL_AlphaEdgeFix((byte*)data, tex->width, tex->height);
	}

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

	GL_SetFilterModes(tex);

	/*if (tex->mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}*/
}

/*
================
CGLRenderer::GL_PadImageW -- return image with width padded up to power-of-two dimentions
================
*/
byte* CGLRenderer::GL_PadImageW(byte* in, int width, int height, byte padbyte)
{
	int i, j, outwidth;
	byte* out, * data;

	if (width == GL_Pad(width))
		return in;

	outwidth = GL_Pad(width);

	out = data = g_MemCache->Hunk_Alloc<byte>(outwidth * height);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
			*out++ = *in++;
		for (; j < outwidth; j++)
			*out++ = padbyte;
	}

	return data;
}

/*
================
CGLRenderer::GL_PadImageH -- return image with height padded up to power-of-two dimentions
================
*/
byte* CGLRenderer::GL_PadImageH(byte* in, int width, int height, byte padbyte)
{
	int i, srcpix, dstpix;
	byte* data, * out;

	if (height == GL_Pad(height))
		return in;

	srcpix = width * height;
	dstpix = width * GL_Pad(height);

	out = data = g_MemCache->Hunk_Alloc<byte>(dstpix);

	for (i = 0; i < srcpix; i++)
		*out++ = *in++;
	for (; i < dstpix; i++)
		*out++ = padbyte;

	return data;
}

/*
===============
CGLRenderer::GL_PadEdgeFixW -- special case of AlphaEdgeFix for textures that only need it because they were padded

operates in place on 32bit data, and expects unpadded height and width values
===============
*/
void CGLRenderer::GL_PadEdgeFixW(byte* data, int width, int height)
{
	byte* src, * dst;
	int i, padw, padh;

	padw = GL_PadConditional(width);
	padh = GL_PadConditional(height);

	//copy last full column to first empty column, leaving alpha byte at zero
	src = data + (width - 1) * 4;
	for (i = 0; i < padh; i++)
	{
		src[4] = src[0];
		src[5] = src[1];
		src[6] = src[2];
		src += padw * 4;
	}

	//copy first full column to last empty column, leaving alpha byte at zero
	src = data;
	dst = data + (padw - 1) * 4;
	for (i = 0; i < padh; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += padw * 4;
		dst += padw * 4;
	}
}

/*
===============
CGLRenderer::GL_PadEdgeFixH -- special case of AlphaEdgeFix for textures that only need it because they were padded

operates in place on 32bit data, and expects unpadded height and width values
===============
*/
void CGLRenderer::GL_PadEdgeFixH(byte* data, int width, int height)
{
	byte* src, * dst;
	int i, padw, padh;

	padw = GL_PadConditional(width);
	padh = GL_PadConditional(height);

	//copy last full row to first empty row, leaving alpha byte at zero
	dst = data + height * padw * 4;
	src = dst - padw * 4;
	for (i = 0; i < padw; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += 4;
		dst += 4;
	}

	//copy first full row to last empty row, leaving alpha byte at zero
	dst = data + (padh - 1) * padw * 4;
	src = data;
	for (i = 0; i < padw; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += 4;
		dst += 4;
	}
}

/*
================
CGLRenderer::GL_LoadImage32 -- handles 32bit source data
================
*/
void CGLRenderer::GL_LoadImage32(CGLTexture* glt, unsigned* data)
{
	int	internalformat, miplevel, mipwidth, mipheight, picmip;

	if (!gl_texture_NPOT)
	{
		// resample up
		data = GL_ResampleTexture(data, glt->width, glt->height, glt->flags & TEXPREF_ALPHA);
		glt->width = GL_Pad(glt->width);
		glt->height = GL_Pad(glt->height);
	}

	// mipmap down
	picmip = (glt->flags & TEXPREF_NOPICMIP) ? 0 : q_max((int)gl_picmip.value, 0);
	mipwidth = GL_SafeTextureSize(glt->width >> picmip);
	mipheight = GL_SafeTextureSize(glt->height >> picmip);
	while ((int)glt->width > mipwidth)
	{
		GL_MipMapW(data, glt->width, glt->height);
		glt->width >>= 1;
		if (glt->flags & TEXPREF_ALPHA)
			GL_AlphaEdgeFix((byte*)data, glt->width, glt->height);
	}
	while ((int)glt->height > mipheight)
	{
		GL_MipMapH(data, glt->width, glt->height);
		glt->height >>= 1;
		if (glt->flags & TEXPREF_ALPHA)
			GL_AlphaEdgeFix((byte*)data, glt->width, glt->height);
	}

	// upload
	GL_Bind(glt);
	internalformat = (glt->flags & TEXPREF_ALPHA) ? gl_alpha_format : gl_solid_format;
	glt->pic.datavec.AddMultipleToTail(glt->source_width * glt->source_height, (byte*)data);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, glt->width, glt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// upload mipmaps
	if (glt->flags & TEXPREF_MIPMAP && !(glt->flags & TEXPREF_WARPIMAGE)) // warp image mipmaps are generated later
	{
		mipwidth = glt->width;
		mipheight = glt->height;

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

	// set filter modes
	GL_SetFilterModes(glt);
}

/*
================
CGLRenderer::GL_LoadImage8 -- handles 8bit source data, then passes it to LoadImage32
================
*/
void CGLRenderer::GL_LoadImage8(CGLTexture* glt, byte* data)
{
	extern cvar_t gl_fullbrights;
	bool padw = false, padh = false;
	byte padbyte;
	unsigned int* usepal;
	int i;

	// HACK HACK HACK -- taken from tomazquake
	if (strstr(glt->identifier, "shot1sid") &&
		glt->width == 32 && glt->height == 32 &&
		g_CRCManager->CRC_Block(data, 1024) == 65393)
	{
		// This texture in b_shell1.bsp has some of the first 32 pixels painted white.
		// They are invisible in software, but look really ugly in GL. So we just copy
		// 32 pixels from the bottom to make it look nice.
		memcpy(data, data + 32 * 31, 32);
	}

	// detect false alpha cases
	if (glt->flags & TEXPREF_ALPHA && !(glt->flags & TEXPREF_CONCHARS))
	{
		for (i = 0; i < (int)(glt->width * glt->height); i++)
			if (data[i] == 255) //transparent index
				break;
		if (i == (int)(glt->width * glt->height))
			glt->flags -= TEXPREF_ALPHA;
	}

	// choose palette and padbyte
	if (glt->flags & TEXPREF_FULLBRIGHT)
	{
		if (glt->flags & TEXPREF_ALPHA)
			usepal = d_8to24table_fbright_fence;
		else
			usepal = d_8to24table_fbright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_NOBRIGHT && gl_fullbrights.value)
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

	// pad each dimention, but only if it's not going to be downsampled later
	if (glt->flags & TEXPREF_PAD)
	{
		if ((int)glt->width < GL_SafeTextureSize(glt->width))
		{
			data = GL_PadImageW(data, glt->width, glt->height, padbyte);
			glt->width = GL_Pad(glt->width);
			padw = true;
		}
		if ((int)glt->height < GL_SafeTextureSize(glt->height))
		{
			data = GL_PadImageH(data, glt->width, glt->height, padbyte);
			glt->height = GL_Pad(glt->height);
			padh = true;
		}
	}

	// convert to 32bit
	data = (byte*)GL_8to32(data, glt->width * glt->height, usepal);

	// fix edges
	if (glt->flags & TEXPREF_ALPHA)
		GL_AlphaEdgeFix(data, glt->width, glt->height);
	else
	{
		if (padw)
			GL_PadEdgeFixW(data, glt->source_width, glt->source_height);
		if (padh)
			GL_PadEdgeFixH(data, glt->source_width, glt->source_height);
	}

	// upload it
	GL_LoadImage32(glt, (unsigned*)data);
}

/*
================
CGLRenderer::GL_LoadLightmap -- handles lightmap data
================
*/
void CGLRenderer::GL_LoadLightmap(CGLTexture* glt, byte* data)
{
	// upload it
	GL_Bind(glt);
	glTexImage2D(GL_TEXTURE_2D, 0, lightmap_bytes, glt->width, glt->height, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, data);

	// set filter modes
	GL_SetFilterModes(glt);
}

/*
================
CGLRenderer::GL_LoadTexture -- Missi: revised (3/18/2022)

Loads an OpenGL texture via string ID or byte data. Can take an empty string if you don't need a name.
================
*/
CGLTexture* CGLRenderer::GL_LoadTexture(model_t* owner, const char* identifier, int width, int height, enum srcformat_t format, byte* data, uintptr_t offset, int flags)
{
	CGLTexture*		glt = NULL;
	int				CRCBlock;
	int				mark = 0;

// Missi: the below two lines used to be an else statement in vanilla GLQuake... with horrible results (3/22/2022)

	switch (format)
	{
		case SRC_INDEXED:
			CRCBlock = g_CRCManager->CRC_Block(data, width * height);
			break;
		case SRC_LIGHTMAP:
			CRCBlock = g_CRCManager->CRC_Block(data, width * height * lightmap_bytes);
			break;
		case SRC_RGBA:
			CRCBlock = g_CRCManager->CRC_Block(data, width * height * 4);
			break;
		default: /* not reachable but avoids compiler warnings */
			CRCBlock = 0;
	}

	if ((flags & TEXPREF_OVERWRITE) && (glt = GL_FindTexture(owner, identifier)))
	{
		if (glt->checksum == CRCBlock)
			return glt;
	}
	else
		glt = GL_NewTexture();

	glt->pic.width = width;
	glt->pic.height = height;

	Q_strlcpy(glt->identifier, identifier, sizeof(glt->identifier));	// Missi: this was Q_strcpy at one point. Caused heap corruption. (4/11/2022)
	glt->owner = owner;
	glt->texnum = texture_extension_number;
	glt->source_offset = offset;
	glt->source_width = width;
	glt->source_height = height;
	glt->width = width;
	glt->height = height;
	glt->flags = flags;
	glt->mipmap = (flags & TEXPREF_MIPMAP) ? true : false;
	glt->source_format = format;
	glt->checksum = CRCBlock;

	//upload it
	mark = g_MemCache->Hunk_LowMark();

	switch (glt->source_format)
	{
	case SRC_INDEXED:
		GL_LoadImage8(glt, data);
		break;
	case SRC_LIGHTMAP:
		GL_LoadLightmap(glt, data);
		break;
	case SRC_RGBA:
		GL_LoadImage32(glt, (unsigned*)data);
		break;
	}

	texture_extension_number++;

	g_MemCache->Hunk_FreeToLowMark(mark);

	return glt;

}

/*
================
CGLRenderer::GL_LoadPicTexture
================
*/
CGLTexture* CGLRenderer::GL_LoadPicTexture (CQuakePic* pic)
{
	uintptr_t offset;

	offset = (uintptr_t)pic->datavec.Base() - (uintptr_t)wad_base;

	return GL_LoadTexture(NULL, "", pic->width, pic->height, SRC_INDEXED, pic->datavec.Base(), TEXPREF_ALPHA);
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

CGLTexture::CGLTexture() :  next(NULL),
    owner(NULL),
    width(0),
    height(0),
    mipmap(false),
	source_width(0),
    source_height(0),
    source_format(SRC_NONE),
    source_offset((uintptr_t)0),
    checksum(0),
    flags(0)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::CGLTexture(CQuakePic qpic, float des_sl, float des_tl, float des_sh, float des_th) : next(NULL),
    owner(NULL),
    width(0),
    height(0),
    mipmap(false),
    source_width(0),
    source_height(0),
    source_format(SRC_NONE),
    source_offset((uintptr_t)0),
    checksum(0),
    flags(0)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::CGLTexture(const CGLTexture& obj) : next(NULL),
    owner(NULL),
    width(0),
    height(0),
    mipmap(false),
    source_width(0),
    source_height(0),
    source_format(SRC_NONE),
    source_offset((uintptr_t)0),
    checksum(0),
    flags(0)
{
	memset(identifier, 0x00, sizeof(identifier));
}

CGLTexture::~CGLTexture()
{
	memset(&pic.datavec, 0, sizeof(CQuakePic));
}
