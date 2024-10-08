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

// refresh.h -- public interface to refresh functions

#define	MAXCLIPPLANES	11

/*
#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96
*/

constexpr unsigned short TOP_RANGE = 16;
constexpr unsigned short BOTTOM_RANGE = 96;

/*
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256
*/

constexpr unsigned short BLOCK_WIDTH = 256;
constexpr unsigned short BLOCK_HEIGHT = 256;

constexpr unsigned short LMBLOCK_WIDTH = 256;
constexpr unsigned short LMBLOCK_HEIGHT = 256;

constexpr unsigned short MAX_LIGHTMAPS = 64;		// Missi: was 64 (6/2/2024)

//=============================================================================

typedef struct efrag_s
{
	struct mleaf_s		*leaf;
	struct efrag_s		*leafnext;
	struct entity_s		*entity;
	struct efrag_s		*entnext;
} efrag_t;

typedef struct entity_s
{
	bool				forcelink;		// model changed

	int						update_type;

	entity_state_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)	
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;	
	struct model_s			*model;			// NULL = no model
	struct efrag_s			*efrag;			// linked list of efrags
	int						frame;
	float					syncbase;		// for client-side animations
	byte					*colormap;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						visframe;		// last frame this entity was
											//  found in an active leaf
											
	int						dlightframe;	// dynamic lighting
	int						dlightbits;
	
// FIXME: could turn these into a union
	int						trivial_accept;
	struct mnode_s			*topnode;		// for bmodels, first world node
											//  that splits bmodel, or NULL if
											//  not split
} entity_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t		vrect;				// subwindow in video for refresh
									// FIXME: not need vrect next field here?
	vrect_t		aliasvrect;			// scaled Alias version
	int			vrectright, vrectbottom;	// right & bottom screen coords
	int			aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about,
										//  for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj, fvrectbottom_adj;
										// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible 
										// 2.0 = 90 degrees
	float		xOrigin;			// should probably allways be 0.5
	float		yOrigin;			// between be around 0.3 to 0.5

	vec3_t		vieworg;
	vec3_t		viewangles;
	
	float		fov_x, fov_y;

	int			ambientlight;
} refdef_t;

enum ERenderContext
{
	RENDER_INVALID = -1,
	RENDER_CORE,
	RENDER_SOFTWARE,
	RENDER_OPENGL
};

//
// refresh
//
extern	int		reinit_surfcache;


extern	refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;

extern	struct texture_s	*r_notexture_mip;

#ifdef GLQUAKE
class CGLTexture;
#else
class CTexture;
#endif

/*
* Missi: core renderer
* 
* There are three renderers present in BananaQuake: the core renderer, the software renderer, and
* the OpenGL renderer.
* 
*/
class CCoreRenderer
{
public:

	CCoreRenderer();

	void R_Init();
	void R_InitTextures();
	void R_InitTurb();
	// void R_InitEfrags();
	void R_DrawEntitiesOnList();
	void R_DrawViewModel();
	void R_DrawBEntitiesOnList();
	void R_EdgeDrawing();
	void R_RenderView_();
	void R_RenderView();		// must set r_refdef first
	void R_ViewChanged(vrect_t* pvrect, int lineadj, float aspect);
	void R_MarkLeaves();
	// called whenever r_refdef or vid change
	// void R_InitSky(struct texture_s* mt);	// called at level load

	void R_AddEfrags(entity_t* ent);
	void R_RemoveEfrags(entity_t* ent);

	void R_NewMap();


	void R_ParseParticleEffect();
	void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
	void R_RocketTrail(vec3_t start, vec3_t end, int type);

#ifdef QUAKE2
	void R_DarkFieldParticles(entity_t* ent);
#endif
	void R_EntityParticles(entity_t* ent);
	void R_BlobExplosion(vec3_t org);
	void R_ParticleExplosion(vec3_t org);
	void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength);
	void R_LavaSplash(vec3_t org);
	void R_TeleportSplash(vec3_t org);

	// void R_PushDlights();
	void R_SetVrect(vrect_t* pvrect, vrect_t* pvrectin, int lineadj);

	static void R_ReadPointFile_f();
	void R_DrawParticles();
	void R_InitParticles();
	void R_ClearParticles();
	void R_PushDlights();
#ifdef GLQUAKE
	CGLTexture* solidskytexture;
	CGLTexture* alphaskytexture;
#else
	CTexture* solidskytexture;
	CTexture* alphaskytexture;
#endif
	float	speedscale;		// for top sky and bottom sky

	void R_StoreEfrags(efrag_t** ppefrag);

#ifndef GLQUAKE
	static void R_TimeRefresh_f();
	void R_TimeGraph();
	void R_PrintAliasStats();
	void R_PrintTimes();
	void R_PrintDSpeeds();
	void R_AnimateLight();
	int R_LightPoint(vec3_t p);
	void R_SetupFrame();
	void R_cshift_f();
	struct msurface_s* warpface;

	void R_AddDynamicLights();

	void R_BuildLightMap();
	void R_EmitEdge(struct msurface_s* pv0, struct msurface_s* pv1);
	void R_ClipEdge(struct msurface_s* pv0, struct msurface_s* pv1, struct clipplane_s* clip);
	void R_SplitEntityOnNode2(struct mnode_s* node);
	void R_MarkLights(struct dlight_s* light, int bit, struct mnode_s* node);
	void R_SplitEntityOnNode(struct mnode_s* node);
	int R_BmodelCheckBBox(struct model_s* clmodel, float* minmaxs);

	void R_GenTile(struct msurface_s* psurf, void* pdest);
#else
	void R_SplitEntityOnNode(mnode_s* node);
#endif

	void R_DrawSurface();

	static unsigned		blocklights[BLOCK_WIDTH * BLOCK_HEIGHT * 3];

	virtual CCoreRenderer* GetRenderer() { return this; }

private:

	CCoreRenderer(const CCoreRenderer& src);

};

extern cvar_t gl_subdivide_size;

//
// surface cache related
//
extern	int		reinit_surfcache;	// if 1, surface cache is currently empty and
extern bool	r_cache_thrash;	// set if thrashing the surface cache

int	D_SurfaceCacheForRes (int width, int height);
void D_FlushCaches ();
void D_DeleteSurfaceCache ();
void D_InitCaches (void *buffer, int size);

//extern ERenderContext R_ResolveRenderer();		// Missi: nasty! gross! horrid! i hate myself! (3/15/2022)

extern CCoreRenderer* g_CoreRenderer;
