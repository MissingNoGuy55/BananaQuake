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
// r_main.c

#include "quakedef.h"

entity_t	r_worldentity;

bool	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

bool	envmap;				// true during envmap command capture 

CGLTexture* currenttexture = NULL;		// to avoid unnecessary texture sets

CGLTexture* cnttextures[2] = {NULL, NULL};     // cached

CGLTexture*			particletexture;	// little dot for particles
CGLTexture*			playertextures[MAX_SCOREBOARD];		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
bool	mirror;
mplane_t	*mirror_plane;

float fog_color_vec[4] = {};
float fog_density = 0.0f;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_lightmap = {"r_lightmap","0"};
cvar_t	r_shadows = {"r_shadows","0"};
cvar_t	r_mirroralpha = {"r_mirroralpha","1"};
cvar_t	r_wateralpha = {"r_wateralpha","1"};
cvar_t	r_dynamic = {"r_dynamic","1"};
cvar_t	r_novis = {"r_novis","0"};

cvar_t	gl_finish = {"gl_finish","0"};
cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_texsort = {"gl_texsort","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};
cvar_t	gl_polyblend = {"gl_polyblend","1"};
cvar_t	gl_flashblend = {"gl_flashblend","1"};
cvar_t	gl_playermip = {"gl_playermip","0"};
cvar_t	gl_nocolors = {"gl_nocolors","0"};
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","0"};
cvar_t	gl_reporttjunctions = {"gl_reporttjunctions","0"};
cvar_t	gl_doubleeyes = {"gl_doubleeys", "1"};

int skytexturenum;

static bool s_bFogColorLerping = false;
static bool s_bFogDensityLerping = false;
static bool s_bFogStartEndLerping = false;

double s_dWorldFogColor[3] = { 0.0, 0.0, 0.0 };
double s_dCurFogDensity = 0.0;
double s_dCurFogStart = 0.0;
double s_dCurFogEnd = 0.0;
double s_dFogLerpTime = 0.0;
double s_dFogLerpDensityTime = 0.0;
double s_dFogLerpStartTime = 0.0;
double s_dFogLerpEndTime = 0.0;

static double s_dFogStartTimeStamp = 0.0;
static double s_dFogEndTimeStamp = 0.0;

static double s_dFogLerpTimeStamp = 0.0f;

unsigned CCoreRenderer::blocklights[BLOCK_WIDTH * BLOCK_HEIGHT * 3];

CCoreRenderer::CCoreRenderer() : solidskytexture(NULL)
{
	solidskytexture = NULL;
	alphaskytexture = NULL;
	speedscale = 1.0f;
	memset(blocklights, 0, sizeof(blocklights));
}

CCoreRenderer::CCoreRenderer(const CCoreRenderer& src)
{
	solidskytexture = NULL;
	alphaskytexture = NULL;
	speedscale = 1.0f;
	memset(blocklights, 0, sizeof(blocklights));
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
bool CGLRenderer::R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void CGLRenderer::R_RotateForEntity (entity_t *e)
{
    glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    glRotatef (e->angles[1],  0, 0, 1);
    glRotatef (-e->angles[0],  0, 1, 0);
    glRotatef (e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t * CGLRenderer::R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = static_cast<msprite_t*>(currententity->model->cache.data);
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void CGLRenderer::R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	float		*up, *right;
	vec3_t		v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = static_cast<msprite_t*>(currententity->model->cache.data);

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	glColor3f (1,1,1);

	GL_DisableMultitexture();

    g_GLRenderer->GL_Bind(frame->gltexture);

	glEnable (GL_ALPHA_TEST);
	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);
	
	glEnd ();

	glDisable (GL_ALPHA_TEST);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

double	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
extern vec3_t	lightcolor;
float	ambientlight;
//float	shadelight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
double	r_avertexnormal_dots[SHADEDOT_QUANT][256] = {
	{1.23,1.30,1.47,1.35,1.56,1.71,1.37,1.38,1.59,1.60,1.79,1.97,1.88,1.92,1.79,1.02,0.93,1.07,0.82,0.87,0.88,0.94,0.96,1.14,1.11,0.82,0.83,0.89,0.89,0.86,0.94,0.91,1.00,1.21,0.98,1.48,1.30,1.57,0.96,1.07,1.14,1.60,1.61,1.40,1.37,1.72,1.78,1.79,1.93,1.99,1.90,1.68,1.71,1.86,1.60,1.68,1.78,1.86,1.93,1.99,1.97,1.44,1.22,1.49,0.93,0.99,0.99,1.23,1.22,1.44,1.49,0.89,0.89,0.97,0.91,0.98,1.19,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.19,0.98,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.87,0.93,0.94,1.02,1.30,1.07,1.35,1.38,1.11,1.56,1.92,1.79,1.79,1.59,1.60,1.72,1.90,1.79,0.80,0.85,0.79,0.93,0.80,0.85,0.77,0.74,0.72,0.77,0.74,0.72,0.70,0.70,0.71,0.76,0.73,0.79,0.79,0.73,0.76,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.26,1.26,1.48,1.23,1.50,1.71,1.14,1.19,1.38,1.46,1.64,1.94,1.87,1.84,1.71,1.02,0.92,1.00,0.79,0.85,0.84,0.91,0.90,0.98,0.99,0.77,0.77,0.83,0.82,0.79,0.86,0.84,0.92,0.99,0.91,1.24,1.03,1.33,0.88,0.94,0.97,1.41,1.39,1.18,1.11,1.51,1.61,1.59,1.80,1.91,1.76,1.54,1.65,1.76,1.70,1.70,1.85,1.85,1.97,1.99,1.93,1.28,1.09,1.39,0.92,0.97,0.99,1.18,1.26,1.52,1.48,0.83,0.85,0.90,0.88,0.93,1.00,0.77,0.73,0.78,0.72,0.71,0.74,0.75,0.79,0.86,0.81,0.75,0.81,0.79,0.96,0.88,0.94,0.86,0.93,0.92,0.85,1.08,1.33,1.05,1.55,1.31,1.01,1.05,1.27,1.31,1.60,1.47,1.70,1.54,1.76,1.76,1.57,0.93,0.90,0.99,0.88,0.88,0.95,0.97,1.11,1.39,1.20,0.92,0.97,1.01,1.10,1.39,1.22,1.51,1.58,1.32,1.64,1.97,1.85,1.91,1.77,1.74,1.88,1.99,1.91,0.79,0.86,0.80,0.94,0.84,0.88,0.74,0.74,0.71,0.82,0.77,0.76,0.70,0.73,0.72,0.73,0.70,0.74,0.85,0.77,0.82,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.34,1.27,1.53,1.17,1.46,1.71,0.98,1.05,1.20,1.34,1.48,1.86,1.82,1.71,1.62,1.09,0.94,0.99,0.79,0.85,0.82,0.90,0.87,0.93,0.96,0.76,0.74,0.79,0.76,0.74,0.79,0.78,0.85,0.92,0.85,1.00,0.93,1.06,0.81,0.86,0.89,1.16,1.12,0.97,0.95,1.28,1.38,1.35,1.60,1.77,1.57,1.33,1.50,1.58,1.69,1.63,1.82,1.74,1.91,1.92,1.80,1.04,0.97,1.21,0.90,0.93,0.97,1.05,1.21,1.48,1.37,0.77,0.80,0.84,0.85,0.88,0.92,0.73,0.71,0.74,0.74,0.71,0.75,0.73,0.79,0.84,0.78,0.79,0.86,0.81,1.05,0.94,0.99,0.90,0.95,0.92,0.86,1.24,1.44,1.14,1.59,1.34,1.02,1.27,1.50,1.49,1.80,1.69,1.86,1.72,1.87,1.80,1.69,1.00,0.98,1.23,0.95,0.96,1.09,1.16,1.37,1.63,1.46,0.99,1.10,1.25,1.24,1.51,1.41,1.67,1.77,1.55,1.72,1.95,1.89,1.98,1.91,1.86,1.97,1.99,1.94,0.81,0.89,0.85,0.98,0.90,0.94,0.75,0.78,0.73,0.89,0.83,0.82,0.72,0.77,0.76,0.72,0.70,0.71,0.91,0.83,0.89,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.46,1.34,1.60,1.16,1.46,1.71,0.94,0.99,1.05,1.26,1.33,1.74,1.76,1.57,1.54,1.23,0.98,1.05,0.83,0.89,0.84,0.92,0.87,0.91,0.96,0.78,0.74,0.79,0.72,0.72,0.75,0.76,0.80,0.88,0.83,0.94,0.87,0.95,0.76,0.80,0.82,0.97,0.96,0.89,0.88,1.08,1.11,1.10,1.37,1.59,1.37,1.07,1.27,1.34,1.57,1.45,1.69,1.55,1.77,1.79,1.60,0.93,0.90,0.99,0.86,0.87,0.93,0.96,1.07,1.35,1.18,0.73,0.76,0.77,0.81,0.82,0.85,0.70,0.71,0.72,0.78,0.73,0.77,0.73,0.79,0.82,0.76,0.83,0.90,0.84,1.18,0.98,1.03,0.92,0.95,0.90,0.86,1.32,1.45,1.15,1.53,1.27,0.99,1.42,1.65,1.58,1.93,1.83,1.94,1.81,1.88,1.74,1.70,1.19,1.17,1.44,1.11,1.15,1.36,1.41,1.61,1.81,1.67,1.22,1.34,1.50,1.42,1.65,1.61,1.82,1.91,1.75,1.80,1.89,1.89,1.98,1.99,1.94,1.98,1.92,1.87,0.86,0.95,0.92,1.14,0.98,1.03,0.79,0.84,0.77,0.97,0.90,0.89,0.76,0.82,0.82,0.74,0.72,0.71,0.98,0.89,0.97,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.60,1.44,1.68,1.22,1.49,1.71,0.93,0.99,0.99,1.23,1.22,1.60,1.68,1.44,1.49,1.40,1.14,1.19,0.89,0.96,0.89,0.97,0.89,0.91,0.98,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.91,0.83,0.89,0.72,0.76,0.76,0.89,0.89,0.82,0.82,0.98,0.96,0.97,1.14,1.40,1.19,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.70,0.72,0.73,0.77,0.76,0.79,0.70,0.72,0.71,0.82,0.77,0.80,0.74,0.79,0.80,0.74,0.87,0.93,0.85,1.23,1.02,1.02,0.93,0.93,0.87,0.85,1.30,1.35,1.07,1.38,1.11,0.94,1.47,1.71,1.56,1.97,1.88,1.92,1.79,1.79,1.59,1.60,1.30,1.35,1.56,1.37,1.38,1.59,1.60,1.79,1.92,1.79,1.48,1.57,1.72,1.61,1.78,1.79,1.93,1.99,1.90,1.86,1.78,1.86,1.93,1.99,1.97,1.90,1.79,1.72,0.94,1.07,1.00,1.37,1.21,1.30,0.86,0.91,0.83,1.14,0.98,0.96,0.82,0.88,0.89,0.79,0.76,0.73,1.07,0.94,1.11,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.74,1.57,1.76,1.33,1.54,1.71,0.94,1.05,0.99,1.26,1.16,1.46,1.60,1.34,1.46,1.59,1.37,1.37,0.97,1.11,0.96,1.10,0.95,0.94,1.08,0.89,0.82,0.88,0.72,0.76,0.75,0.80,0.80,0.88,0.87,0.91,0.83,0.87,0.72,0.76,0.74,0.83,0.84,0.78,0.79,0.96,0.89,0.92,0.98,1.23,1.05,0.86,0.92,0.95,1.11,0.98,1.22,1.03,1.34,1.42,1.14,0.79,0.77,0.84,0.78,0.76,0.82,0.82,0.89,0.97,0.90,0.70,0.71,0.71,0.73,0.72,0.74,0.73,0.76,0.72,0.86,0.81,0.82,0.76,0.79,0.77,0.73,0.90,0.95,0.86,1.18,1.03,0.98,0.92,0.90,0.83,0.84,1.19,1.17,0.98,1.15,0.97,0.89,1.42,1.65,1.44,1.93,1.83,1.81,1.67,1.61,1.36,1.41,1.32,1.45,1.58,1.57,1.53,1.74,1.70,1.88,1.94,1.81,1.69,1.77,1.87,1.79,1.89,1.92,1.98,1.99,1.98,1.89,1.65,1.80,1.82,1.91,1.94,1.75,1.61,1.50,1.07,1.34,1.27,1.60,1.45,1.55,0.93,0.99,0.90,1.35,1.18,1.07,0.87,0.93,0.96,0.85,0.82,0.77,1.15,0.99,1.27,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.86,1.71,1.82,1.48,1.62,1.71,0.98,1.20,1.05,1.34,1.17,1.34,1.53,1.27,1.46,1.77,1.60,1.57,1.16,1.38,1.12,1.35,1.06,1.00,1.28,0.97,0.89,0.95,0.76,0.81,0.79,0.86,0.85,0.92,0.93,0.93,0.85,0.87,0.74,0.78,0.74,0.79,0.82,0.76,0.79,0.96,0.85,0.90,0.94,1.09,0.99,0.81,0.85,0.89,0.95,0.90,0.99,0.94,1.10,1.24,0.98,0.75,0.73,0.78,0.74,0.72,0.77,0.76,0.82,0.89,0.83,0.73,0.71,0.71,0.71,0.70,0.72,0.77,0.80,0.74,0.90,0.85,0.84,0.78,0.79,0.75,0.73,0.92,0.95,0.86,1.05,0.99,0.94,0.90,0.86,0.79,0.81,1.00,0.98,0.91,0.96,0.89,0.83,1.27,1.50,1.23,1.80,1.69,1.63,1.46,1.37,1.09,1.16,1.24,1.44,1.49,1.69,1.59,1.80,1.69,1.87,1.86,1.72,1.82,1.91,1.94,1.92,1.95,1.99,1.98,1.91,1.97,1.89,1.51,1.72,1.67,1.77,1.86,1.55,1.41,1.25,1.33,1.58,1.50,1.80,1.63,1.74,1.04,1.21,0.97,1.48,1.37,1.21,0.93,0.97,1.05,0.92,0.88,0.84,1.14,1.02,1.34,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.94,1.84,1.87,1.64,1.71,1.71,1.14,1.38,1.19,1.46,1.23,1.26,1.48,1.26,1.50,1.91,1.80,1.76,1.41,1.61,1.39,1.59,1.33,1.24,1.51,1.18,0.97,1.11,0.82,0.88,0.86,0.94,0.92,0.99,1.03,0.98,0.91,0.90,0.79,0.84,0.77,0.79,0.84,0.77,0.83,0.99,0.85,0.91,0.92,1.02,1.00,0.79,0.80,0.86,0.88,0.84,0.92,0.88,0.97,1.10,0.94,0.74,0.71,0.74,0.72,0.70,0.73,0.72,0.76,0.82,0.77,0.77,0.73,0.74,0.71,0.70,0.73,0.83,0.85,0.78,0.92,0.88,0.86,0.81,0.79,0.74,0.75,0.92,0.93,0.85,0.96,0.94,0.88,0.86,0.81,0.75,0.79,0.93,0.90,0.85,0.88,0.82,0.77,1.05,1.27,0.99,1.60,1.47,1.39,1.20,1.11,0.95,0.97,1.08,1.33,1.31,1.70,1.55,1.76,1.57,1.76,1.70,1.54,1.85,1.97,1.91,1.99,1.97,1.99,1.91,1.77,1.88,1.85,1.39,1.64,1.51,1.58,1.74,1.32,1.22,1.01,1.54,1.76,1.65,1.93,1.70,1.85,1.28,1.39,1.09,1.52,1.48,1.26,0.97,0.99,1.18,1.00,0.93,0.90,1.05,1.01,1.31,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.97,1.92,1.88,1.79,1.79,1.71,1.37,1.59,1.38,1.60,1.35,1.23,1.47,1.30,1.56,1.99,1.93,1.90,1.60,1.78,1.61,1.79,1.57,1.48,1.72,1.40,1.14,1.37,0.89,0.96,0.94,1.07,1.00,1.21,1.30,1.14,0.98,0.96,0.86,0.91,0.83,0.82,0.88,0.82,0.89,1.11,0.87,0.94,0.93,1.02,1.07,0.80,0.79,0.85,0.82,0.80,0.87,0.85,0.93,1.02,0.93,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.82,0.76,0.79,0.72,0.73,0.76,0.89,0.89,0.82,0.93,0.91,0.86,0.83,0.79,0.73,0.76,0.91,0.89,0.83,0.89,0.89,0.82,0.82,0.76,0.72,0.76,0.86,0.83,0.79,0.82,0.76,0.73,0.94,1.00,0.91,1.37,1.21,1.14,0.98,0.96,0.88,0.89,0.96,1.14,1.07,1.60,1.40,1.61,1.37,1.57,1.48,1.30,1.78,1.93,1.79,1.99,1.92,1.90,1.79,1.59,1.72,1.79,1.30,1.56,1.35,1.38,1.60,1.11,1.07,0.94,1.68,1.86,1.71,1.97,1.68,1.86,1.44,1.49,1.22,1.44,1.49,1.22,0.99,0.99,1.23,1.19,0.98,0.97,0.97,0.98,1.19,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.94,1.97,1.87,1.91,1.85,1.71,1.60,1.77,1.58,1.74,1.51,1.26,1.48,1.39,1.64,1.99,1.97,1.99,1.70,1.85,1.76,1.91,1.76,1.70,1.88,1.55,1.33,1.57,0.96,1.08,1.05,1.31,1.27,1.47,1.54,1.39,1.20,1.11,0.93,0.99,0.90,0.88,0.95,0.88,0.97,1.32,0.92,1.01,0.97,1.10,1.22,0.84,0.80,0.88,0.79,0.79,0.85,0.86,0.92,1.02,0.94,0.82,0.76,0.77,0.72,0.73,0.70,0.72,0.71,0.74,0.74,0.88,0.81,0.85,0.75,0.77,0.82,0.94,0.93,0.86,0.92,0.92,0.86,0.85,0.79,0.74,0.79,0.88,0.85,0.81,0.82,0.83,0.77,0.78,0.73,0.71,0.75,0.79,0.77,0.74,0.77,0.73,0.70,0.86,0.92,0.84,1.14,0.99,0.98,0.91,0.90,0.84,0.83,0.88,0.97,0.94,1.41,1.18,1.39,1.11,1.33,1.24,1.03,1.61,1.80,1.59,1.91,1.84,1.76,1.64,1.38,1.51,1.71,1.26,1.50,1.23,1.19,1.46,0.99,1.00,0.91,1.70,1.85,1.65,1.93,1.54,1.76,1.52,1.48,1.26,1.28,1.39,1.09,0.99,0.97,1.18,1.31,1.01,1.05,0.90,0.93,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.86,1.95,1.82,1.98,1.89,1.71,1.80,1.91,1.77,1.86,1.67,1.34,1.53,1.51,1.72,1.92,1.91,1.99,1.69,1.82,1.80,1.94,1.87,1.86,1.97,1.59,1.44,1.69,1.05,1.24,1.27,1.49,1.50,1.69,1.72,1.63,1.46,1.37,1.00,1.23,0.98,0.95,1.09,0.96,1.16,1.55,0.99,1.25,1.10,1.24,1.41,0.90,0.85,0.94,0.79,0.81,0.85,0.89,0.94,1.09,0.98,0.89,0.82,0.83,0.74,0.77,0.72,0.76,0.73,0.75,0.78,0.94,0.86,0.91,0.79,0.83,0.89,0.99,0.95,0.90,0.90,0.92,0.84,0.86,0.79,0.75,0.81,0.85,0.80,0.78,0.76,0.77,0.73,0.74,0.71,0.71,0.73,0.74,0.74,0.71,0.76,0.72,0.70,0.79,0.85,0.78,0.98,0.92,0.93,0.85,0.87,0.82,0.79,0.81,0.89,0.86,1.16,0.97,1.12,0.95,1.06,1.00,0.93,1.38,1.60,1.35,1.77,1.71,1.57,1.48,1.20,1.28,1.62,1.27,1.46,1.17,1.05,1.34,0.96,0.99,0.90,1.63,1.74,1.50,1.80,1.33,1.58,1.48,1.37,1.21,1.04,1.21,0.97,0.97,0.93,1.05,1.34,1.02,1.14,0.84,0.88,0.92,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.74,1.89,1.76,1.98,1.89,1.71,1.93,1.99,1.91,1.94,1.82,1.46,1.60,1.65,1.80,1.79,1.77,1.92,1.57,1.69,1.74,1.87,1.88,1.94,1.98,1.53,1.45,1.70,1.18,1.32,1.42,1.58,1.65,1.83,1.81,1.81,1.67,1.61,1.19,1.44,1.17,1.11,1.36,1.15,1.41,1.75,1.22,1.50,1.34,1.42,1.61,0.98,0.92,1.03,0.83,0.86,0.89,0.95,0.98,1.23,1.14,0.97,0.89,0.90,0.78,0.82,0.76,0.82,0.77,0.79,0.84,0.98,0.90,0.98,0.83,0.89,0.97,1.03,0.95,0.92,0.86,0.90,0.82,0.86,0.79,0.77,0.84,0.81,0.76,0.76,0.72,0.73,0.70,0.72,0.71,0.73,0.73,0.72,0.74,0.71,0.78,0.74,0.72,0.75,0.80,0.76,0.94,0.88,0.91,0.83,0.87,0.84,0.79,0.76,0.82,0.80,0.97,0.89,0.96,0.88,0.95,0.94,0.87,1.11,1.37,1.10,1.59,1.57,1.37,1.33,1.05,1.08,1.54,1.34,1.46,1.16,0.99,1.26,0.96,1.05,0.92,1.45,1.55,1.27,1.60,1.07,1.34,1.35,1.18,1.07,0.93,0.99,0.90,0.93,0.87,0.96,1.27,0.99,1.15,0.77,0.82,0.85,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.60,1.78,1.68,1.93,1.86,1.71,1.97,1.99,1.99,1.97,1.93,1.60,1.68,1.78,1.86,1.61,1.57,1.79,1.37,1.48,1.59,1.72,1.79,1.92,1.90,1.38,1.35,1.60,1.23,1.30,1.47,1.56,1.71,1.88,1.79,1.92,1.79,1.79,1.30,1.56,1.35,1.37,1.59,1.38,1.60,1.90,1.48,1.72,1.57,1.61,1.79,1.21,1.00,1.30,0.89,0.94,0.96,1.07,1.14,1.40,1.37,1.14,0.96,0.98,0.82,0.88,0.82,0.89,0.83,0.86,0.91,1.02,0.93,1.07,0.87,0.94,1.11,1.02,0.93,0.93,0.82,0.87,0.80,0.85,0.79,0.80,0.85,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.72,0.76,0.73,0.82,0.79,0.76,0.73,0.79,0.76,0.93,0.86,0.91,0.83,0.89,0.89,0.82,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.44,1.19,1.22,0.99,0.98,1.49,1.44,1.49,1.22,0.99,1.23,0.98,1.19,0.97,1.21,1.30,1.00,1.37,0.94,1.07,1.14,0.98,0.96,0.86,0.91,0.83,0.88,0.82,0.89,1.11,0.94,1.07,0.73,0.76,0.79,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.46,1.65,1.60,1.82,1.80,1.71,1.93,1.91,1.99,1.94,1.98,1.74,1.76,1.89,1.89,1.42,1.34,1.61,1.11,1.22,1.36,1.50,1.61,1.81,1.75,1.15,1.17,1.41,1.18,1.19,1.42,1.44,1.65,1.83,1.67,1.94,1.81,1.88,1.32,1.58,1.45,1.57,1.74,1.53,1.70,1.98,1.69,1.87,1.77,1.79,1.92,1.45,1.27,1.55,0.97,1.07,1.11,1.34,1.37,1.59,1.60,1.35,1.07,1.18,0.86,0.93,0.87,0.96,0.90,0.93,0.99,1.03,0.95,1.15,0.90,0.99,1.27,0.98,0.90,0.92,0.78,0.83,0.77,0.84,0.79,0.82,0.86,0.73,0.71,0.73,0.72,0.70,0.73,0.72,0.76,0.81,0.76,0.76,0.82,0.77,0.89,0.85,0.82,0.75,0.80,0.80,0.94,0.88,0.94,0.87,0.95,0.96,0.88,0.72,0.74,0.76,0.83,0.78,0.84,0.79,0.87,0.91,0.83,0.89,0.98,0.92,1.23,1.34,1.05,1.16,0.99,0.96,1.46,1.57,1.54,1.33,1.05,1.26,1.08,1.37,1.10,0.98,1.03,0.92,1.14,0.86,0.95,0.97,0.90,0.89,0.79,0.84,0.77,0.82,0.76,0.82,0.97,0.89,0.98,0.71,0.72,0.74,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.34,1.51,1.53,1.67,1.72,1.71,1.80,1.77,1.91,1.86,1.98,1.86,1.82,1.95,1.89,1.24,1.10,1.41,0.95,0.99,1.09,1.25,1.37,1.63,1.55,0.96,0.98,1.16,1.05,1.00,1.27,1.23,1.50,1.69,1.46,1.86,1.72,1.87,1.24,1.49,1.44,1.69,1.80,1.59,1.69,1.97,1.82,1.94,1.91,1.92,1.99,1.63,1.50,1.74,1.16,1.33,1.38,1.58,1.60,1.77,1.80,1.48,1.21,1.37,0.90,0.97,0.93,1.05,0.97,1.04,1.21,0.99,0.95,1.14,0.92,1.02,1.34,0.94,0.86,0.90,0.74,0.79,0.75,0.81,0.79,0.84,0.86,0.71,0.71,0.73,0.76,0.73,0.77,0.74,0.80,0.85,0.78,0.81,0.89,0.84,0.97,0.92,0.88,0.79,0.85,0.86,0.98,0.92,1.00,0.93,1.06,1.12,0.95,0.74,0.74,0.78,0.79,0.76,0.82,0.79,0.87,0.93,0.85,0.85,0.94,0.90,1.09,1.27,0.99,1.17,1.05,0.96,1.46,1.71,1.62,1.48,1.20,1.34,1.28,1.57,1.35,0.90,0.94,0.85,0.98,0.81,0.89,0.89,0.83,0.82,0.75,0.78,0.73,0.77,0.72,0.76,0.89,0.83,0.91,0.71,0.70,0.72,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
	{1.26,1.39,1.48,1.51,1.64,1.71,1.60,1.58,1.77,1.74,1.91,1.94,1.87,1.97,1.85,1.10,0.97,1.22,0.88,0.92,0.95,1.01,1.11,1.39,1.32,0.88,0.90,0.97,0.96,0.93,1.05,0.99,1.27,1.47,1.20,1.70,1.54,1.76,1.08,1.31,1.33,1.70,1.76,1.55,1.57,1.88,1.85,1.91,1.97,1.99,1.99,1.70,1.65,1.85,1.41,1.54,1.61,1.76,1.80,1.91,1.93,1.52,1.26,1.48,0.92,0.99,0.97,1.18,1.09,1.28,1.39,0.94,0.93,1.05,0.92,1.01,1.31,0.88,0.81,0.86,0.72,0.75,0.74,0.79,0.79,0.86,0.85,0.71,0.73,0.75,0.82,0.77,0.83,0.78,0.85,0.88,0.81,0.88,0.97,0.90,1.18,1.00,0.93,0.86,0.92,0.94,1.14,0.99,1.24,1.03,1.33,1.39,1.11,0.79,0.77,0.84,0.79,0.77,0.84,0.83,0.90,0.98,0.91,0.85,0.92,0.91,1.02,1.26,1.00,1.23,1.19,0.99,1.50,1.84,1.71,1.64,1.38,1.46,1.51,1.76,1.59,0.84,0.88,0.80,0.94,0.79,0.86,0.82,0.77,0.76,0.74,0.74,0.71,0.73,0.70,0.72,0.82,0.77,0.85,0.74,0.70,0.73,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00}
};

double	*shadedots = r_avertexnormal_dots[0];

int	lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void CGLRenderer::GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum)
{
	float	vertcolor[4];
	trivertx_t *verts;
	int		*order;
	int		count;

	lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			// normals and vertexes come from the frame list
			vertcolor[0] = shadedots[verts->lightnormalindex] * lightcolor[0];
			vertcolor[1] = shadedots[verts->lightnormalindex] * lightcolor[1];
			vertcolor[2] = shadedots[verts->lightnormalindex] * lightcolor[2];
			glColor4f (vertcolor[0], vertcolor[1], vertcolor[2], vertcolor[3]);
			glVertex3f (verts->v[0], verts->v[1], verts->v[2]);
			verts++;
		} while (--count);

		glEnd ();
	}
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void CGLRenderer::GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	trivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			glVertex3fv (point);

			verts++;
		} while (--count);

		glEnd ();
	}	
}



/*
=================
R_SetupAliasFrame

=================
*/
void CGLRenderer::R_SetupAliasFrame (int frame, aliashdr_t *paliashdr)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose);
}



/*
=================
R_DrawAliasModel

=================
*/
void CGLRenderer::R_DrawAliasModel (entity_t *e)
{
	int			i;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	float		an;
	int			anim;
	int			quantizedangle;
	
	//
	// locate the proper data
	//
	paliashdr = Mod_Extradata<aliashdr_t>(currententity->model);

	// if the initial trace is completely black, try again from above
	// this helps with models whose origin is slightly below ground level
	// (e.g. some of the candles in the DOTM start map)
	if (!R_LightPoint(e->origin))
	{
		vec3_t lpos;
		VectorCopy(e->origin, lpos);
		lpos[2] += e->model->maxs[2] * 0.5f;
		R_LightPoint(lpos);
	}

	clmodel = currententity->model;

	VectorAdd (currententity->origin, clmodel->mins, mins);
	VectorAdd (currententity->origin, clmodel->maxs, maxs);

	if (R_CullBox (mins, maxs))
		return;

	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//

	ambientlight = R_LightPoint (currententity->origin);

	// allways give the gun some light
	if (e == &cl.viewent)
	{
		add = 72.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	for (i=0 ; i<MAX_DLIGHTS ; i++)
	{
		if (cl_dlights[i].die >= cl.time)
		{
			VectorSubtract(currententity->origin, cl_dlights[i].origin, dist);
			add = cl_dlights[i].radius - VectorLength(dist);
			if (add > 0) {
				VectorMA(lightcolor, add, cl_dlights[i].color, lightcolor);
			}
		}
	}

	// clamp lighting so it doesn't overbright as much
	add = 288.0f / (lightcolor[0] + lightcolor[1] + lightcolor[2]);
	if (add < 1.0f)
		VectorScale(lightcolor, add, lightcolor);

	// ZOID: never allow players to go totally black
	if (currententity > cl_entities && currententity <= cl_entities + cl.maxclients)
	{
		add = 24.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl")
		|| !strcmp (clmodel->name, "progs/flame.mdl") )
		ambientlight = 256; // shadelight = 256;

	quantizedangle = ((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1);
	
	an = (quantizedangle / 16.0) * 2.0 * 3.14159;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize(shadevector);

	shadedots = r_avertexnormal_dots[quantizedangle];
	VectorScale(lightcolor, 1.0f / 200.0f, lightcolor);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

    glPushMatrix ();
	R_RotateForEntity (e);

	if (!strcmp (clmodel->name, "progs/eyes.mdl") && gl_doubleeyes.value) {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - (22 + 8));
// double size of eyes, since they are really hard to see in gl
		glScalef (paliashdr->scale[0]*2, paliashdr->scale[1]*2, paliashdr->scale[2]*2);
	} else {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	}

	anim = (int)(cl.time*10) & 3;
    g_GLRenderer->GL_Bind(paliashdr->gltextures[currententity->skinnum][anim]);

	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (currententity->colormap != vid.colormap && !gl_nocolors.value)
	{
		if ((uintptr_t)e >= (uintptr_t)&cl_entities[1] && (uintptr_t)e <= (uintptr_t)&cl_entities[cl.maxclients] /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		    g_GLRenderer->GL_Bind(playertextures[e - cl_entities - 1]);
	}

	if (gl_smoothmodels.value)
		glShadeModel (GL_SMOOTH);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame (currententity->frame, paliashdr);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glShadeModel (GL_FLAT);
	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPopMatrix ();

	if (r_shadows.value)
	{
		glPushMatrix ();
		R_RotateForEntity (e);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glColor4f (0,0,0,0.5);
		GL_DrawAliasShadow (paliashdr, lastposenum);
		glEnable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
		glColor4f (1,1,1,1);
		glPopMatrix ();
	}

}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void CGLRenderer::R_DrawEntitiesOnList ()
{
	int		i;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;

		case mod_brush:
			R_DrawBrushModel (currententity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void CGLRenderer::R_DrawViewModel ()
{
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;

	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value)
		return;

	if (envmap)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			ambientlight += add;
	}

	if (currententity->model->type != mod_alias)
		return;

	// hack the depth range to prevent view model from poking into walls

#ifndef WIN64
	glDepthRange (gldepthmin, gldepthmin + 0.3f * (gldepthmax-gldepthmin));
#else
	glDepthRange((long double)gldepthmin, (long double)gldepthmin + (long double)0.3 * ((long double)gldepthmax - (long double)gldepthmin));
#endif

	R_DrawAliasModel (currententity);
	glDepthRange (gldepthmin, gldepthmax);
}


/*
============
R_PolyBlend
============
*/
void CGLRenderer::R_PolyBlend ()
{
	if (!gl_polyblend.value)
		return;
	if (!v_blend[3])
		return;

	GL_DisableMultitexture();

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);

    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);
}

int CGLRenderer::SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void CGLRenderer::R_SetFrustum ()
{
	int		i;

	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);
	}
	else
	{
		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void CGLRenderer::R_SetupFrame ()
{
// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


void CGLRenderer::MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void CGLRenderer::R_SetupGL ()
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	glViewport (glx + x, gly + y2, w, h);
    screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
//	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
    MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  4096);

	if (mirror)
	{
		if (mirror_plane->normal[2])
			glScalef (1, -1, 1);
		else
			glScalef (-1, 1, 1);
		glCullFace(GL_BACK);
	}
	else
		glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void CGLRenderer::R_RenderScene ()
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	g_SoundSystem->S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GL_DisableMultitexture();

	R_RenderDlights ();

	g_CoreRenderer->R_DrawParticles ();

	
	for (int i = 0; i < svs.maxclients; i++)
	{
		client_t* pPlayer = &svs.clients[i];

		if (!pPlayer)
			break;

		if (pPlayer)
			Draw_PrintPlayerName(pPlayer, i, pPlayer->name);
	}

#ifdef GLTEST
	Test_Draw ();
#endif

}


/*
=============
R_Clear
=============
*/
void CGLRenderer::R_Clear ()
{
	if (r_mirroralpha.value != 1.0)
	{
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 0.5;
		glDepthFunc (GL_LEQUAL);
	}
	else if (gl_ztrick.value)
	{
		static int trickframe;

		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0.0f;
			gldepthmax = 0.49999f;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1.0f;
			gldepthmax = 0.5f;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRange (gldepthmin, gldepthmax);
}

/*
=============
R_Mirror
=============
*/
void CGLRenderer::R_Mirror ()
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;

	if (!mirror)
		return;

	memcpy (r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asin (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2 (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}

	gldepthmin = 0.5;
	gldepthmax = 1;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	R_RenderScene ();
	R_DrawWaterSurfaces ();

	gldepthmin = 0;
	gldepthmax = 0.5;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	// blend on top
	glEnable (GL_BLEND);
	glMatrixMode(GL_PROJECTION);
	if (mirror_plane->normal[2])
		glScalef (1,-1,1);
	else
		glScalef (-1,1,1);
	glCullFace(GL_FRONT);
	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf (r_base_world_matrix);

	glColor4f (1,1,1,r_mirroralpha.value);
	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	glDisable (GL_BLEND);
	glColor4f (1,1,1,1);
}

static double lastLerpColorTick = 0.0;
static double lastLerpDensityTick = 0.0;
static double lastLerpStartTick = 0.0;
static double lastLerpEndTick = 0.0;

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void CGLRenderer::R_RenderView ()
{
	double	time1, time2;
	GLfloat colors[4] = {(GLfloat) 0.0, (GLfloat) 0.0, (GLfloat) 1, (GLfloat) 0.20};

	if (r_norefresh.value)
		return;

	if (!cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		glFinish ();
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	if (gl_finish.value)
		glFinish ();

	R_Clear ();

	// render normal view

/***** Experimental silly looking fog ******
****** Use r_fullbright if you enable ******
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, colors);
	glFogf(GL_FOG_END, 512.0);
	glEnable(GL_FOG);
********************************************/

    if (!sv->DoesLevelHaveFog())
    {
        glDisable(GL_FOG);
    }
    else
    {
		// Missi: fog lerping code (6/18/2024)
        if ((!s_bFogColorLerping) && (level_fog_color_r.value != level_fog_color_goal_r.value ||
                                      level_fog_color_g.value != level_fog_color_goal_g.value ||
                                      level_fog_color_b.value != level_fog_color_goal_b.value))
		{
			s_dFogLerpTime = Cvar_VariableValue("fog_lerp_time");

			lastLerpColorTick = 0.0;
			s_bFogColorLerping = true;
		}
        if ((!s_bFogDensityLerping) && (level_fog_density_goal.value != level_fog_density.value))
		{
			s_dFogLerpDensityTime = Cvar_VariableValue("fog_lerp_time");

            lastLerpDensityTick = 0.0;
			s_bFogDensityLerping = true;
        }
        if ((!s_bFogStartEndLerping) && (level_fog_start_goal.value != level_fog_start.value ||
                                         level_fog_end_goal.value != level_fog_end.value))
		{
			s_dFogLerpStartTime = Cvar_VariableValue("fog_lerp_time");
            s_dFogLerpEndTime = Cvar_VariableValue("fog_lerp_time");

            s_dFogStartTimeStamp = level_fog_start.value;
            s_dFogEndTimeStamp = level_fog_end.value;

			lastLerpStartTick = 0.0;
			lastLerpEndTick = 0.0;
			s_bFogStartEndLerping = true;
        }

		if (s_bFogColorLerping)
		{
            lastLerpColorTick += host->host_frametime;

            double red = Lerp<double>(level_fog_color_r.value, level_fog_color_goal_r.value, (lastLerpColorTick / s_dFogLerpTime));
			double green = Lerp<double>(level_fog_color_g.value, level_fog_color_goal_g.value, (lastLerpColorTick / s_dFogLerpTime));
			double blue = Lerp<double>(level_fog_color_b.value, level_fog_color_goal_b.value, (lastLerpColorTick / s_dFogLerpTime));

            fog_color_vec[0] = red;
            fog_color_vec[1] = green;
            fog_color_vec[2] = blue;

            //Con_DPrintf("fog color lerp: %f %f %f\n", fog_color_vec[0], fog_color_vec[1], fog_color_vec[2]);

			if (lastLerpColorTick > s_dFogLerpTime)
			{
				s_bFogColorLerping = false;
				fog_color_vec[0] = level_fog_color_goal_r.value;
				fog_color_vec[1] = level_fog_color_goal_g.value;
				fog_color_vec[2] = level_fog_color_goal_b.value;

				Cvar_SetValue("fog_r", fog_color_vec[0]);
				Cvar_SetValue("fog_g", fog_color_vec[1]);
				Cvar_SetValue("fog_b", fog_color_vec[2]);
			}
		}
		if (s_bFogDensityLerping)
		{
			lastLerpDensityTick += host->host_frametime;

            double val = Lerp<double>(level_fog_density.value, level_fog_density_goal.value, (lastLerpDensityTick / s_dFogLerpDensityTime));

            if (lastLerpColorTick > s_dFogLerpDensityTime)
            {
                s_bFogDensityLerping = false;

				s_dCurFogDensity = level_fog_density_goal.value;
				Cvar_SetValue("fog_density", s_dCurFogDensity);
			}
		}
		if (s_bFogStartEndLerping)
		{
			lastLerpStartTick += host->host_frametime;
			lastLerpEndTick += host->host_frametime;

            static double start = 0.0;
            static double end = 0.0;

            if (level_fog_start_goal.value >= level_fog_start.value)
                start = Lerp<double>(level_fog_start.value, level_fog_start_goal.value, lastLerpStartTick / s_dFogLerpStartTime);
            else
                start = Lerp<double>(level_fog_start_goal.value, level_fog_start.value, lastLerpStartTick / s_dFogLerpStartTime);

            if (level_fog_end_goal.value >= level_fog_end.value)
                end = Lerp<double>(level_fog_end.value, level_fog_end_goal.value, lastLerpEndTick / s_dFogLerpEndTime);
            else
                end = Lerp<double>(level_fog_end_goal.value, level_fog_end.value, lastLerpEndTick / s_dFogLerpEndTime);

            s_dCurFogStart = start;
            s_dCurFogEnd = end;

            Con_DPrintf("fog start: %4.2f\nfog end: %4.2f\n", s_dCurFogStart, s_dCurFogEnd);

            if (lastLerpColorTick > s_dFogLerpStartTime && lastLerpEndTick >= s_dFogLerpEndTime)
			{
				s_dCurFogStart = level_fog_start_goal.value;
                s_dCurFogEnd = level_fog_end_goal.value;

                Cvar_SetValue("fog_start", s_dCurFogStart);
                Cvar_SetValue("fog_end", s_dCurFogEnd);

				s_bFogStartEndLerping = false;
			}
		}

        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fog_color_vec);
        glFogf(GL_FOG_DENSITY, s_dCurFogDensity);
        glFogf(GL_FOG_START, s_dCurFogStart);
        glFogf(GL_FOG_END, s_dCurFogEnd);
        glEnable(GL_FOG);
    }

	R_RenderScene();
	R_DrawViewModel();
	R_DrawWaterSurfaces();

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();

	if (r_speeds.value)
	{
//		glFinish ();
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}

ERenderContext R_ResolveRenderer()
{
#ifndef GLQUAKE
	g_SoftwareRenderer = new CSoftwareRenderer;
	g_SoftwareRenderer->R_InitTextures();
	return RENDER_SOFTWARE;
#else
	g_GLRenderer = new CGLRenderer;
	g_GLRenderer->R_InitTextures();
	return RENDER_OPENGL;
#endif
}
