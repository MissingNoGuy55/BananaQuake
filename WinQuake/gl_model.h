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

#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mvertex_s
{
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!

typedef struct texture_s
{
	char		name[16];
	unsigned	width, height;
	class CGLTexture*	gltexture;
	struct msurface_s	*texturechain;	// for gl_texsort drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} texture_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct
{
	float		vecs[2][4];
	float		mipadjust;
	texture_t	*texture;
	int			flags;
} mtexinfo_t;

#define	VERTEXSIZE	7

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	int		flags;			// for SURF_UNDERWATER
	float	verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates

	glpoly_t	*polys;				// multiple if warped
	struct	msurface_s	*texturechain;

	mtexinfo_t	*texinfo;
	
// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	int			cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	bool	cached_dlight;				// true if dynamic light in cache
	byte		*samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	byte		*compressed_vis;
	efrag_t		*efrags;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
struct hull_t
{
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
};

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int		width;
	int		height;
	float	up, down, left, right;
	CGLTexture* gltexture;
} mspriteframe_t;

typedef struct
{
	int				numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	void				*cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct
{
	int					firstpose;
	int					numposes;
	float				interval;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
	int					facesfront;
	int					vertindex[3];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t
	int					commands;	// gl command list with embedded s/t
	CGLTexture*			gltextures[MAX_SKINS][4];
	int					texels[MAX_SKINS];	// only for player skins
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS	1024
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048
extern	aliashdr_t	*pheader;
extern	stvert_t	stverts[MAXALIASVERTS];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	trivertx_t	*poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum {mod_brush, mod_sprite, mod_alias} modtype_t;

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail

typedef struct model_s
{
	char		name[MAX_QPATH] = {};
	bool	needload = false;		// bmodels and sprites don't cache normally

	modtype_t	type = {};
	int			numframes = 0;
	synctype_t	synctype = {};
	
	int			flags = 0;

//
// volume occupied by the model graphics
//		
	vec3_t		mins = {}, maxs = {};
	float		radius = 0.0f;

//
// solid volume for clipping 
//
	bool	clipbox = false;
	vec3_t		clipmins = {}, clipmaxs = {};

//
// brush model
//
	int			firstmodelsurface = 0, nummodelsurfaces = 0;

	int			numsubmodels = 0;
	dmodel_t* submodels = {};

	int			numplanes = 0;
	mplane_t* planes = {};

	int			numleafs = 0;		// number of visible leafs, not counting 0
	mleaf_t* leafs = {};

	int			numvertexes = 0;
	mvertex_t	*vertexes = {};

	int			numedges = 0;
	medge_t		*edges = {};

	int			numnodes = 0;
	mnode_t		*nodes = {};

	int			numtexinfo = 0;
	mtexinfo_t	*texinfo = {};

	int			numsurfaces = 0;
	msurface_t	*surfaces = {};

	int			numsurfedges = 0;
	int			*surfedges = {};

	int			numclipnodes = 0;
	dclipnode_t	*clipnodes = {};

	int			nummarksurfaces;
	msurface_t	**marksurfaces = {};

	hull_t		hulls[MAX_MAP_HULLS] = {};

	int			numtextures = 0;
	texture_t	**textures = {};

	byte		*visdata = {};
	byte		*lightdata = {};
	char		*entities = {};

//
// additional model data
//
	template<typename T>
	static cache_user_s<T>	cache;		// only access through Mod_Extradata

} model_t;

extern model_t* loadmodel;
extern char	loadname[32];	// for hunk tags

extern byte	mod_novis[MAX_MAP_LEAFS / 8];

#define	MAX_MOD_KNOWN	512
extern model_t	mod_known[MAX_MOD_KNOWN];
extern int		mod_numknown;

extern cvar_t gl_subdivide_size;

template<typename T>
cache_user_s<T> model_t::cache = {};

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, bool crash);

template<typename T>
model_t* Mod_LoadModel(model_t* mod, bool crash);

void Mod_LoadSpriteModel(model_t* mod, void* buffer);
void Mod_LoadBrushModel(model_t* mod, void* buffer);
void Mod_LoadAliasModel(model_t* mod, void* buffer);

template<typename T>
T*	Mod_Extradata (model_t *mod);	// handles caching

void	Mod_TouchModel (char *name);

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
template<typename T>
model_t* Mod_LoadModel(model_t* mod, bool crash)
{
	T* d;
	unsigned* buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

	if (!mod->needload)
	{
		if (mod->type == mod_alias)
		{
			d = g_MemCache->Cache_Check<T>(&mod->cache<T>);
			if (d)
				return mod;
		}
		else
			return mod;		// not cached at all
	}

	//
	// because the world is so huge, load it one piece at a time
	//
	if (!crash)
	{

	}

	//
	// load the file
	//
	buf = COM_LoadStackFile<unsigned>(mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error("Mod_NumForName: %s not found", mod->name);
		return NULL;
	}

	//
	// allocate a new model
	//
	common->COM_FileBase(mod->name, loadname);

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader
	mod->needload = false;

	switch (LittleLong(*(unsigned*)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel(mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;

	default:
		Mod_LoadBrushModel(mod, buf);
		break;
	}

	return mod;
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
template<typename T>
T* Mod_Extradata(model_t* mod)
{
	T* r;

	r = g_MemCache->Cache_Check<T>(&mod->cache<T>);
	if (r)
		return r;

	Mod_LoadModel<T>(mod, true);

	if (!mod->cache<T>.data)
		Sys_Error("Mod_Extradata: caching failed");
	return mod->cache<T>.data;
}

#endif	// __MODEL__
