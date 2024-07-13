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
// disable data conversion warnings

#pragma once
#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86


#ifdef WIN64
#pragma warning(disable : 4267)		// Missi: x64 (12/5/2022)
#endif

#pragma warning(disable : 4051)     // ALPHA
#endif
//#include <GL/glew.h>

#define	GL_UNUSED_TEXTURE	(~(GLuint)0)

#ifdef _WIN32
#include <windows.h>
#include "winquake.h"
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/Xxf86dga.h>    // Missi (4/30/2023)
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef _WIN32
// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *,
                    const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *,
                    const GLclampf *);
typedef void (APIENTRY *TEXSUBIMAGEPTR)(int, int, int, int, int, int, int, int, void *);

extern	BINDTEXFUNCPTR bindTexFunc;
extern	DELTEXFUNCPTR delTexFunc;
extern	TEXSUBIMAGEPTR TexSubImage2DFunc;
#endif

extern	int texture_extension_number;
extern	int		texture_mode;

extern	float	gldepthmin, gldepthmax;

extern cvar_t gl_ztrick;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

extern glvert_t glv;

extern	int glx, gly, glwidth, glheight;

#ifdef _WIN32
extern	PROC QglArrayElementEXT;
extern	PROC QglColorPointerEXT;
extern	PROC QglTexturePointerEXT;
extern	PROC QglVertexPointerEXT;
#endif

// r_local.h -- private refresh defs

constexpr double	ALIAS_BASE_SIZE_RATIO = (1.0 / 11.0);
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
constexpr int		MAX_LBM_HEIGHT = 480;

constexpr int		TILE_SIZE = 128;		// size of textures generated by R_GenTiledSurf

constexpr int		SKYSHIFT = 7;
constexpr int		SKYSIZE = (1 << SKYSHIFT);
constexpr int		SKYMASK = (SKYSIZE - 1);

constexpr double	BACKFACE_EPSILON = 0.01;

constexpr int		MAX_GLTEXTURES = 1024;

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	byte				data[4];	// width*height elements
} surfcache_t;


typedef struct
{
	pixel_t		*surfdat;	// destination for generated surface
	int			rowbytes;	// destination logical width in bytes
	msurface_t	*surf;		// description for surface to generate
	fixed8_t	lightadj[MAXLIGHTMAPS];
							// adjust for lightmap levels for dynamic lighting
	texture_t	*texture;	// corrected for animating textures
	int			surfmip;	// mipmapped ratio of surface texels / world pixels
	int			surfwidth;	// in mipmapped texels
	int			surfheight;	// in mipmapped texels
} drawsurf_t;


typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;

typedef struct glRect_s {
	unsigned short l, t, w, h;
} glRect_t;

constexpr unsigned int TEXPREF_NONE			= 0x0000;
constexpr unsigned int TEXPREF_MIPMAP		= 0x0001;	// generate mipmaps
// TEXPREF_NEAREST and TEXPREF_LINEAR aren't supposed to be ORed with TEX_MIPMAP
constexpr unsigned int TEXPREF_LINEAR		= 0x0002;	// force linear
constexpr unsigned int TEXPREF_NEAREST		= 0x0004;	// force nearest
constexpr unsigned int TEXPREF_ALPHA		= 0x0008;	// allow alpha
constexpr unsigned int TEXPREF_PAD			= 0x0010;	// allow padding
constexpr unsigned int TEXPREF_PERSIST		= 0x0020;	// never free
constexpr unsigned int TEXPREF_OVERWRITE	= 0x0040;	// overwrite existing same-name texture
constexpr unsigned int TEXPREF_NOPICMIP		= 0x0080;	// always load full-sized
constexpr unsigned int TEXPREF_FULLBRIGHT	= 0x0100;	// use fullbright mask palette
constexpr unsigned int TEXPREF_NOBRIGHT		= 0x0200;	// use nobright mask palette
constexpr unsigned int TEXPREF_CONCHARS		= 0x0400;	// use conchars palette
constexpr unsigned int TEXPREF_WARPIMAGE	= 0x0800;	// resize this texture when warpimagesize changes

extern unsigned short	d_8to16table[256];
extern unsigned int		d_8to24table[256];
extern unsigned int		d_8to24table_wad[MAX_MAP_TEXTURES][256];
extern unsigned int		d_8to24table_fbright[256];
extern unsigned int		d_8to24table_fbright_fence[256];
extern unsigned int     d_8to24table_fbright_fence_wad[MAX_MAP_TEXTURES][256];
extern unsigned int		d_8to24table_nobright[256];
extern unsigned int		d_8to24table_nobright_fence[256];
extern unsigned int     d_8to24table_nobright_fence_wad[MAX_MAP_TEXTURES][256];
extern unsigned int		d_8to24table_conchars[256];
extern unsigned int		d_8to24table_shirt[256];
extern unsigned int		d_8to24table_pants[256];
extern unsigned char    d_15to8table[65536];

extern double s_dWorldFogColor[3];			// Missi: used to keep track of the world fog settings for when the player is not in a fog volume (6/21/2024)
extern double s_dCurFogDensity;				// Missi: current fog density, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)
extern double s_dCurFogStart;				// Missi: current fog starting distance, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)
extern double s_dCurFogEnd;					// Missi: current fog ending distance, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)
extern double s_dFogLerpTime;				// Missi: current fog lerp time, is used as the alpha for the lerp in R_RenderView (6/21/2024)
extern double s_dFogLerpDensityTime;		// Missi: current fog density, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)
extern double s_dFogLerpStartTime;			// Missi: current fog density, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)
extern double s_dFogLerpEndTime;			// Missi: current fog density, is the result of the lerp in R_RenderView and fed to glFogf (6/21/2024)

// Missi: BananaQuake stuff (2/17/22)

void IN_ActivateMouse (void);
void IN_DeactivateMouse (void);

class CGLTexture
{
public:
	CGLTexture();
	CGLTexture(CQuakePic qpic, float sl = 0, float tl = 0, float sh = 0, float th = 0);
	~CGLTexture();

	CGLTexture* next;
	model_t* owner;
	CQuakePic			pic;

	GLuint				texnum;
	char				identifier[64];
	unsigned int		width, height;
	bool				mipmap;

	int					source_width, source_height;
	srcformat_t			source_format;
	uintptr_t			source_offset;

	unsigned int		checksum;
	unsigned int		flags;

	byte				palette[256][3];

private:
	CGLTexture(const CGLTexture& obj);

};

#pragma pack(push, 1)
struct int24
{
	unsigned int _num : 24;
};
#pragma pack(pop)

class CGLRenderer : public CCoreRenderer
{
public:

	CGLRenderer();
	~CGLRenderer();

	void GL_AlphaEdgeFix(byte* data, int width, int height);

	void GL_Bind(CGLTexture* tex);

	int Scrap_AllocBlock(int w, int h, int* x, int* y);

	void Scrap_Upload(void);

	int GL_Pad(int s);

	int GL_SafeTextureSize(int s);

	unsigned short* GL_8to16(byte* in, int pixels, unsigned short* usepal);

	int24* GL_8to24(byte* in, int pixels, int24* usepal);

	static unsigned* GL_8to32(byte* in, int pixels, unsigned int* usepal);

	int GL_PadConditional(int s);

    CQuakePic* Draw_PicFromWad(const char* name, const char* wadname);

	CQuakePic* Draw_PicFromWad_GoldSrc(const char* name, const char* wadname);

	CQuakePic* Draw_CachePic(const char* path);

	void Draw_CharToConback(int num, byte* dest);

	static void Draw_TextureMode_f(void);

	void R_SplitEntityOnNode(mnode_t* node);

	int AllocBlock(int w, int h, int* x, int* y);

	virtual void Draw_Init(void);
	virtual void Draw_Character(int x, int y, int num);
	virtual void Draw_String(int x, int y, const char* str);
	virtual void Draw_DebugChar(char num);
	virtual void Draw_AlphaPic(int x, int y, CQuakePic* pic, float alpha);
	virtual void Draw_Pic(int x, int y, CQuakePic* pic);
    virtual void Draw_TGAPic(int x, int y, CGLTexture *glt);
	virtual void Draw_TransPic(int x, int y, CQuakePic* pic);
	virtual void Draw_TransPicTranslate(int x, int y, CQuakePic* pic, byte* translation);
	virtual void Draw_ConsoleBackground(int lines);
	virtual void Draw_TileClear(int x, int y, int w, int h);
	virtual void Draw_Fill(int x, int y, int w, int h, int c);
	virtual void Draw_FadeScreen(void);
	virtual void Draw_BeginDisc(void);
	virtual void Draw_EndDisc(void);

	void GL_Init(void);

	void GL_BeginRendering (int *x, int *y, int *width, int *height);
    void GL_EndRendering (void);
    
	void GL_Set2D(void);

	CGLTexture* GL_FindTexture(model_t* model, const char* identifier);
	void GL_FreeTextureForModel(model_t* mod);
	void GL_FreeTexture(CGLTexture* kill);
	void GL_DeleteTexture(CGLTexture* texture);

	unsigned* GL_MipMapW(unsigned* data, int width, int height);
	unsigned* GL_MipMapH(unsigned* data, int width, int height);

	unsigned* GL_ResampleTexture(unsigned* in, int inwidth, int inheight, bool alpha);

	void GL_Resample8BitTexture(byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight);
	void GL_SelectTexture(GLenum target);
    CGLTexture* GL_LoadPicTexture(CQuakePic* pic, const char* wadName);
	byte* GL_PadImageW(byte* in, int width, int height, byte padbyte);
	byte* GL_PadImageH(byte* in, int width, int height, byte padbyte);
	void GL_PadEdgeFixW(byte* data, int width, int height);
	void GL_PadEdgeFixH(byte* data, int width, int height);

	void GL_LoadImage32(CGLTexture* glt, unsigned* data);
	void GL_LoadImage8(CGLTexture* glt, byte* data);
	void GL_LoadImage8_WAD(CGLTexture* glt, byte* data, unsigned* palette);
	void GL_LoadLightmap(CGLTexture* glt, byte* data);

    CGLTexture* GL_LoadTexture(model_t* owner, const char* identifier, int width, int height, enum srcformat_t format, byte* data, uintptr_t offset, int flags = TEXPREF_NONE, byte* palette = nullptr);

	void GL_SetCanvas(canvastype newcanvas);

	void BuildSurfaceDisplayList(msurface_t* fa);

	void GL_MipMap(byte* in, int width, int height);
	void GL_MipMap8Bit(byte* in, int width, int height);
	void GL_SetFilterModes(CGLTexture* glt);

	void GL_Upload8(byte* data, int width, int height, bool alpha);

	void GL_Upload32(CGLTexture* tex, unsigned* data);
	void GL_BuildLightmaps(void);
	void GL_CreateSurfaceLightmap(msurface_t* surf);
	void GL_SubdivideSurface(msurface_t* fa);

	CGLTexture* GL_NewTexture();

	void EmitWaterPolys(msurface_t* fa);
	void EmitSkyPolys(msurface_t* fa);
	void EmitBothSkyLayers(msurface_t* fa);

	void BoundPoly(int numverts, float* verts, vec3_t mins, vec3_t maxs);
	void SubdividePolygon(int numverts, float* verts);

	int RecursiveLightPoint(vec3_t color, mnode_t* node, vec3_t rayorg, vec3_t start, vec3_t end, float* maxdist);

	void R_Init(void);
	void R_InitParticleTexture(void);
	void R_InitTextures(void);
	void R_DrawBrushModel(entity_t* e);
	void R_DrawSkyChain_Q1(msurface_t* s);
	void R_DrawSkyChain_Q2(msurface_t* s);
	void R_ClearSkyBox(void);
	void MakeSkyVec(float s, float t, int axis);
	void R_DrawSkyBox(void);
	void R_LoadSkys();
	void DrawSkyPolygon(int nump, vec3_t vecs);
	void ClipSkyPolygon(int nump, vec3_t vecs, int stage);
	void R_DrawWorld(void);
	void R_DrawViewModel(void);
	void R_RecursiveWorldNode(mnode_t* node);
	void R_RenderBrushPoly(msurface_t* fa);
    void R_InitSky(texture_t* mt, const char* wadName);
	void R_AddDynamicLights(msurface_t* surf);
	void R_BuildLightMap(msurface_t* surf, byte* dest, int stride);
	void R_BlendLightmaps(void);
	void R_DrawAliasModel(entity_t* e);
	void R_DrawWaterSurfaces(void);
	void R_RenderScene(void);
	void R_PushDlights(void);
	void R_RenderView(void);
	void R_SetupFrame(void);
	void R_NewMap(void);
	void R_AddEfrags(entity_t* ent);

	int R_LightPoint(vec3_t p);
	void R_MarkLights(dlight_t* light, int bit, mnode_t* node);
	void R_AnimateLight(void);
	void R_RenderDlights(void);

	void R_Envmap_f(void);

	void DrawTextureChains(void);
    void DrawGLWaterPoly(glpoly_t* p);
    void DrawGLWaterPolyLightmap(glpoly_t* p);

	texture_t* R_TextureAnimation(texture_t* base);

	CGLTexture gltextures[MAX_GLTEXTURES];

	void R_RenderDynamicLightmaps(msurface_t* fa);
	void R_TimeRefresh_f(void);
	void R_MarkLeaves(void);
	void R_TranslatePlayerSkin(int playernum);
	void R_DrawSequentialPoly(msurface_t* s);
	void R_Mirror(void);
	void R_RenderDlight(dlight_t* light);
	void R_StoreEfrags(efrag_t** ppefrag);

	void AddLightBlend(float r, float g, float b, float a2);
	void DrawGLPoly(glpoly_t* p);

	void GL_DisableMultitexture(void);
	void GL_EnableMultitexture(void);
	void GL_SetFrustum(float fovx, float fovy);

	void R_MirrorChain(msurface_t* s);
	mspriteframe_t* R_GetSpriteFrame(entity_t* currententity);
	void R_DrawSpriteModel(entity_t* e);

// -------------------------------------

	bool R_CullBox(vec3_t mins, vec3_t maxs);
	void R_RotateForEntity(entity_t* e);

	void GL_DrawAliasFrame(aliashdr_t* paliashdr, int posenum);
	void GL_DrawAliasShadow(aliashdr_t* paliashdr, int posenum);

	void R_SetupAliasFrame(int frame, aliashdr_t* paliashdr);
	void R_DrawEntitiesOnList(void);
	void R_PolyBlend(void);
	int SignbitsForPlane(mplane_t* out);
	void R_SetFrustum(void);

    void R_UpdateWarpTextures();

	void MYgluPerspective(GLdouble fovy, GLdouble aspect,
		GLdouble zNear, GLdouble zFar);

	void R_SetupGL(void);
	void R_Clear(void);

	virtual CGLRenderer* GetRenderer() { return dynamic_cast<CGLRenderer*>(g_CoreRenderer); }
    virtual const bool UsesQuake2Skybox() const { return usesQ2Sky; }
    virtual void SetUsesQuake2Skybox(bool bUsing) { usesQ2Sky = bUsing; }

	CQuakePic* GetLoadingDisc() const { return draw_disc; }
	CQuakePic* GetBackTile() const { return draw_backtile; }

private:

	CGLRenderer(const CGLRenderer& src);

	int			skytexturenum;

	glpoly_t*	lightmap_polys[MAX_LIGHTMAPS];
	bool		lightmap_modified[MAX_LIGHTMAPS];
	glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];
	int			lightmap_count;
	int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];	// Missi: changed from a 2D array (12/6/2022)

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmaps[4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];

	// For gl_texsort 0
	msurface_t* skychain;
	msurface_t* waterchain;

	int			lightmap_bytes;		// 1, 2, or 4

	CGLTexture* lightmap_textures[MAX_LIGHTMAPS];

	unsigned	blocklights[BLOCK_WIDTH*BLOCK_HEIGHT*3];
	int			active_lightmaps;

	CGLTexture* free_gltextures;
	CGLTexture* active_gltextures;

	CGLTexture* translate_texture;
	CGLTexture* char_texture;

	int last_lightmap_allocated;

	byte* draw_chars;				// 8*8 graphic characters
	CQuakePic* draw_disc;
	CQuakePic* draw_backtile;

	int		numgltextures;

	int		gl_filter_min;
	int		gl_filter_max;

	int		texels;

	int		gl_lightmap_format;
	int		gl_solid_format;
	int		gl_alpha_format;
	char	q2SkyName[64];
	bool	usesQ2Sky;
};

class COpenGLPic
{
public:
    CGLTexture* tex;
    float	sl, tl, sh, th;
};

extern	bool	gl_texture_NPOT;
extern	GLint	gl_hardware_maxsize;

//====================================================


extern	entity_t	r_worldentity;
extern	bool	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;


//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	texture_t	*r_notexture_mip;
extern	int			d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	bool		envmap;
extern	CGLTexture* currenttexture;
extern	CGLTexture* cnttextures[2];
extern	CGLTexture* particletexture;
extern	CGLTexture* playertextures[MAX_SCOREBOARD];

extern	int		skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_shadows;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;
extern	cvar_t	r_scale;

extern	cvar_t	gl_clear;
extern	cvar_t	gl_cull;
extern	cvar_t	gl_poly;
extern	cvar_t	gl_texsort;
extern	cvar_t	gl_smoothmodels;
extern	cvar_t	gl_affinemodels;
extern	cvar_t	gl_polyblend;
extern	cvar_t	gl_keeptjunctions;
extern	cvar_t	gl_reporttjunctions;
extern	cvar_t	gl_flashblend;
extern	cvar_t	gl_nocolors;
extern	cvar_t	gl_doubleeyes;
extern	cvar_t	gl_fullbrights;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;

extern	cvar_t	gl_max_size;
extern	cvar_t	gl_playermip;

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	bool	mirror;
extern	mplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

extern	const char*		gl_vendor;
extern	const char*		gl_renderer;
extern	const char*		gl_version;
extern	const char*		gl_extensions;

#ifdef __linux__
typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;
#endif

typedef struct {
	modestate_t	type;
	int			width;
	int			height;
	int			modenum;
	int			dib;
	int			fullscreen;
	int			bpp;
	int			halfscreen;
	char		modedesc[17];
} vmode_t;

void R_TranslatePlayerSkin (int playernum);

#ifdef _WIN32
extern vmode_t* GetVideoModes();
#elif __linux__
extern XF86VidModeModeInfo** GetVideoModes();
#endif

// Multitexture
#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F

// Missi: modified (4/21/2023)
#if !(_WIN32) && !defined APIENTRY
#define APIENTRY /* */
#endif

typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
extern lpMTexFUNC qglMTexCoord2fSGIS;
extern lpSelTexFUNC qglSelectTextureSGIS;

extern bool gl_mtexable;

extern CGLRenderer* g_GLRenderer;	// Missi (2/21/2022)

extern cvar_t level_fog_color_r;
extern cvar_t level_fog_color_g;
extern cvar_t level_fog_color_b;
extern cvar_t level_fog_color_goal_r;
extern cvar_t level_fog_color_goal_g;
extern cvar_t level_fog_color_goal_b;
extern cvar_t level_fog_density;
extern cvar_t level_fog_density_goal;
extern cvar_t level_fog_start;
extern cvar_t level_fog_start_goal;
extern cvar_t level_fog_end;
extern cvar_t level_fog_end_goal;
extern cvar_t level_fog_force;
extern cvar_t level_fog_lerp_time;

// Missi: used to access FOV settings in menu.cpp (6/28/2024)
extern cvar_t scr_fov;

extern float fog_color_vec[4];

extern COpenGLPic* SysErrorTex;                // Missi: WALLET (6/14/2024)
extern byte* SysErrorTexBuf;
