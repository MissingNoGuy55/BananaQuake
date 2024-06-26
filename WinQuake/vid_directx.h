#pragma once

// Display an error based on HRESULT value
#define THROW_IF_FAIL(r)								\
		if (r != S_OK)									\
		{												\
			Sys_Error(__func__, __LINE__, __FILE__, r);	\
		}

#define TEXPREF_NONE			0x0000
#define TEXPREF_MIPMAP			0x0001	// generate mipmaps
// TEXPREF_NEAREST and TEXPREF_LINEAR aren't supposed to be ORed with TEX_MIPMAP
#define TEXPREF_LINEAR			0x0002	// force linear
#define TEXPREF_NEAREST			0x0004	// force nearest
#define TEXPREF_ALPHA			0x0008	// allow alpha
#define TEXPREF_PAD			0x0010	// allow padding
#define TEXPREF_PERSIST			0x0020	// never free
#define TEXPREF_OVERWRITE		0x0040	// overwrite existing same-name texture
#define TEXPREF_NOPICMIP		0x0080	// always load full-sized
#define TEXPREF_FULLBRIGHT		0x0100	// use fullbright mask palette
#define TEXPREF_NOBRIGHT		0x0200	// use nobright mask palette
#define TEXPREF_CONCHARS		0x0400	// use conchars palette
#define TEXPREF_WARPIMAGE		0x0800	// resize this texture when warpimagesize changes

#define	MAX_LBM_HEIGHT		480
#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)

template<typename T>
using shared_ptr = std::shared_ptr<T>;

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
	struct particle_s* next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;

typedef struct dxRect_s {
	unsigned short l, t, w, h;
} dxRect_t;

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

// Used to send per-vertex data to the vertex shader.
struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};

// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
class IDeviceNotify
{
public:
	virtual void OnDeviceLost() = 0;
	virtual void OnDeviceRestored() = 0;
};

struct DisplaySize
{
	UINT Width;
	UINT Height;
};

class CDXTexture
{
public:
	CDXTexture();
	CDXTexture(CQuakePic qpic, float sl = 0, float tl = 0, float sh = 0, float th = 0);
	~CDXTexture();

	CDXTexture*			next;
	model_t*			owner;
	CQuakePic			pic;

	int					texnum;
	char				identifier[64];
	unsigned int		width, height;
	bool				mipmap;

	int					source_width, source_height;
	srcformat_t			source_format;
	uintptr_t			source_offset;

	unsigned int		checksum;
	unsigned int		flags;

private:
	CDXTexture(const CDXTexture& obj);

};

class CDXRenderer : public CCoreRenderer
{
public:
	CDXRenderer();

	// System resources for cube geometry.
	ModelViewProjectionConstantBuffer	m_constantBufferData;
	UINT								m_indexCount;

	ID3D11InputLayout*		m_inputLayout;
	ID3D11Buffer*			m_vertexBuffer;
	ID3D11Buffer*			m_indexBuffer;
	ID3D11VertexShader*		m_vertexShader;
	ID3D11PixelShader*		m_pixelShader;
	ID3D11Buffer*			m_constantBuffer;

	bool					m_loadingComplete;
	float					m_degreesPerSecond;
	bool					m_tracking;

	HRESULT					CreateDeviceResources();
	void					CreateDeviceDependentResources();
	void					SetWindow(HWND window);
	void					Render();
	void					ReleaseEverything();

	ID2D1Bitmap*			LoadBitmapTexture(const wchar_t* texName, UINT32 sizeX, UINT32 sizeY, D2D1_BITMAP_PROPERTIES& rBitmapProperties,
		IWICImagingFactory* pWICImagingFactory, ID2D1Bitmap* renderBitmap, ID2D1HwndRenderTarget* pRenderTarget);

	const bool				IsDirectXActive() const { return dx_active; }

	// D3D Accessors.
	ID3D11Device*			GetD3DDevice() const { return m_d3dDevice; }
	ID3D11DeviceContext*	GetD3DDeviceContext() const { return m_d3dContext; }
	IDXGISwapChain1*		GetSwapChain() const { return m_swapChain; }
	D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
	ID3D11RenderTargetView* GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return m_d3dDepthStencilView; }
	D3D11_VIEWPORT			GetScreenViewport() const { return m_screenViewport; }
	DirectX::XMFLOAT4X4		GetOrientationTransform3D() const { return m_orientationTransform3D; }

	// D2D Accessors.
	ID2D1Factory*			GetD2DFactory() const { return m_d2dFactory; }
	ID2D1Device*			GetD2DDevice() const { return m_d2dDevice; }
	ID2D1DeviceContext*		GetD2DDeviceContext() const { return m_d2dContext; }
	ID2D1Bitmap*			GetD2DTargetBitmap() const { return m_d2dTargetBitmap; }
	ID2D1HwndRenderTarget*	GetD2DRenderTarget() const { return m_d2dRenderTargetView; }
	IDWriteFactory*			GetDWriteFactory() const { return m_dwriteFactory; }
	IWICImagingFactory*		GetWicImagingFactory() const { return m_wicFactory; }
	D2D1::Matrix3x2F		GetOrientationTransform2D() const { return m_orientationTransform2D; }

	ID3D11Buffer*			GetVertexBuffer() { return m_vertexBuffer; }

	// The size of the render target, in pixels.
	DisplaySize				GetOutputSize() const { return m_outputSize; }

	// The size of the render target, in dips.
	DisplaySize				GetLogicalSize() const { return m_logicalSize; }
	float					GetDpi() const { return m_effectiveDpi; }

	CQVector<ID2D1BitmapBrush*>	pBitmapBrushes;
	CQVector<ID2D1Bitmap*>		pBitmaps;

	CQVector<ID3D11Texture3D*>	pTextures;

	HRESULT CALLBACK		RenderLoop();

	CQuakePic*				Draw_CachePic(const char* pic)					{ return nullptr; };
	CQuakePic*				Draw_PicFromWad(const char* pic)				{ return nullptr; };
	void					Draw_TransPic(int x, int y, CQuakePic* pic)		{};
	void					Draw_TransPicTranslate(int x, int y, CQuakePic* pic, byte* table)	{};
	void					Draw_Character(int x, int y, const char ch)		{};
	void					Draw_String(int x, int y, const char* str)		{};
	void					Draw_Pic(int x, int y, CQuakePic* pic)			{};
	void					Draw_Fill(int x, int y, int w, int h, int c)	{};
	void					Draw_TileClear(int x, int y, int w, int h)		{};

	void					Draw_ConsoleBackground(int lines)				{};
	void					Draw_FadeScreen()								{};

	CQuakePic*				GetLoadingDisc() { return draw_disc; }
	void					Sbar_DrawDisc(int x, int y, CQuakePic* pic) {};

	HRESULT					CompileShaderFromFile(LPCWCH szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	void 					R_Init(void);
	void 					R_InitParticleTexture(void);
	void 					R_AnimateLight(void);
	void 					AddLightBlend(float r, float g, float b, float a2);
	void 					R_RenderDlight(dlight_t* light);
	void 					R_RenderDlights(void);
	void 					R_MarkLights(dlight_t* light, int bit, mnode_t* node);
	void 					R_PushDlights(void);
	int 					RecursiveLightPoint(vec3_t color, mnode_t* node, vec3_t rayorg, vec3_t start, vec3_t end, float* maxdist);
	int 					R_LightPoint(vec3_t p);
	void 					R_NewMap(void);

	void 					Draw_Init(void);

	CDXTexture*				DX_FindTexture(model_t* model, const char* identifier);
	CDXTexture*				DX_LoadTexture(model_t* owner, const char* identifier, int width, int height, enum srcformat_t format, byte* data, uintptr_t offset, int flags);

	CDXTexture*				DX_NewTexture();

	unsigned*				DX_8to32(byte* in, int pixels, unsigned int* usepal);
	unsigned*				DX_MipMapW(unsigned* data, int width, int height);
	unsigned*				DX_MipMapH(unsigned* data, int width, int height);
	unsigned*				DX_ResampleTexture(unsigned* in, int inwidth, int inheight, bool alpha);

	byte*					DX_PadImageW(byte* in, int width, int height, byte padbyte);
	byte*					DX_PadImageH(byte* in, int width, int height, byte padbyte);

	int						DX_PadConditional(int s);

	void 					DX_PadEdgeFixW(byte* data, int width, int height);
	void 					DX_PadEdgeFixH(byte* data, int width, int height);

	void 					DX_LoadImage8(CDXTexture* glt, byte* data);
	void 					DX_LoadLightmap(CDXTexture* glt, byte* data);
	void 					DX_AlphaEdgeFix(byte* data, int width, int height);
	void 					DX_Bind(CDXTexture* tex);
	void 					DX_LoadImage32(CDXTexture* glt, unsigned* data);

	void					DX_DeleteTexture(CDXTexture* kill);

	void					DX_FreeTexture(CDXTexture* kill);

	void					DX_FreeTextureForModel(model_t* mod);

	void					DX_Resample8BitTexture(byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight);
	int						DX_Pad(int s);

	int						DX_SafeTextureSize(int s);

	void					DX_BuildLightmaps(void);

	void					BuildSurfaceDisplayList(msurface_t* fa);

	void					R_DrawWaterSurfaces(void);

	void					DrawTextureChains(void);

	void					R_DrawBrushModel(entity_t* e);

	void					R_RecursiveWorldNode(mnode_t* node);

	void					R_DrawWorld(void);

	void					R_MarkLeaves(void);

	int						AllocBlock(int w, int h, int* x, int* y);

	void					DX_CreateSurfaceLightmap(msurface_t* surf);

	void					Scrap_Upload(void);

	void					R_AddDynamicLights(msurface_t* surf);

	void					R_BuildLightMap(msurface_t* surf, byte* dest, int stride);

	texture_t*				R_TextureAnimation(texture_t* base);

	void					R_DrawSequentialPoly(msurface_t* s);

	void					DrawDXWaterPoly(dxpoly_t* p);

	void					DrawDXWaterPolyLightmap(dxpoly_t* p);

	void					BoundPoly(int numverts, float* verts, vec3_t mins, vec3_t maxs);

	void					SubdividePolygon(int numverts, float* verts);

	void					DX_SubdivideSurface(msurface_t* fa);

	void					EmitWaterPolys(msurface_t* fa);

	void					EmitSkyPolys(msurface_t* fa);

	void					EmitBothSkyLayers(msurface_t* fa);

	void					R_DrawSkyChain_Q1(msurface_t* s);

	void					R_LoadSkys(void);

	void					DrawSkyPolygon(int nump, vec3_t vecs);

	void					ClipSkyPolygon(int nump, vec3_t vecs, int stage);

	void					R_DrawSkyChain_Q2(msurface_t* s);

	void					R_ClearSkyBox(void);

	void					MakeSkyVec(float s, float t, int axis);

	void					R_DrawSkyBox(void);

	void					R_InitSky(texture_t* mt);

	void					R_UpdateWarpTextures();

	void					R_SplitEntityOnNode(mnode_t* node);

	void					R_AddEfrags(entity_t* ent);

	void					R_StoreEfrags(efrag_t** ppefrag);

	void					R_RenderView(void);

	void					R_RenderScene(void);

	void					R_Clear(void);

	void					R_Mirror(void);

	bool					R_CullBox(vec3_t mins, vec3_t maxs);

	void					R_RotateForEntity(entity_t* e);

	mspriteframe_t*			R_GetSpriteFrame(entity_t* currententity);

	void					R_DrawSpriteModel(entity_t* e);
	void					DX_DrawAliasFrame(aliashdr_t* paliashdr, int posenum);

	void					R_DrawAliasShadow(aliashdr_t* paliashdr, int posenum);

	void					R_SetupAliasFrame(int frame, aliashdr_t* paliashdr);

	void					R_DrawAliasModel(entity_t* e);

	void					R_DrawEntitiesOnList(void);

	void					R_DrawViewModel(void);

	void					R_PolyBlend(void);

	int						SignbitsForPlane(mplane_t* out);

	void					R_SetFrustum(void);
	void					R_SetupFrame(void);
	void					MYDXPerspective(double fovy, double aspect, double zNear, double zFar);
	void					R_SetupGL(void);
	void					R_BlendLightmaps(void);
	void					R_RenderBrushPoly(msurface_t* fa);
	void					R_RenderDynamicLightmaps(msurface_t* fa);
	void					R_MirrorChain(msurface_t* s);

	void					SetUsesQuake2Skybox(bool bUses) { usesQ2Sky = bUses; }
	bool					UsesQuake2Skybox() const { return usesQ2Sky; }

	DirectX::XMMATRIX		m_World;
	DirectX::XMMATRIX		m_View;
	DirectX::XMMATRIX		m_Projection;

	D3D_DRIVER_TYPE			m_driverType;

private:

	void					UpdateRenderTargetSize();
	void					HandleDeviceLost();

	// Cached device properties.
	D3D_FEATURE_LEVEL		m_d3dFeatureLevel;
	DisplaySize				m_d3dRenderTargetSize;
	DisplaySize				m_d2dRenderTargetSize;
	DisplaySize				m_outputSize;
	DisplaySize				m_logicalSize;
	unsigned int			m_nativeOrientation;
	unsigned int			m_currentOrientation;
	float					m_dpi;

	// Transforms used for display orientation.
	D2D1::Matrix3x2F		m_orientationTransform2D;
	DirectX::XMFLOAT4X4		m_orientationTransform3D;

	// Direct2D drawing components.
	ID2D1Factory1*			m_d2dFactory;
	ID2D1Device*			m_d2dDevice;
	ID2D1DeviceContext*		m_d2dContext;
	ID2D1Bitmap*			m_d2dTargetBitmap;
	ID2D1HwndRenderTarget*	m_d2dRenderTargetView;

	// DirectWrite drawing components.
	IDWriteFactory*			m_dwriteFactory;
	IWICImagingFactory*		m_wicFactory;

	// Direct3D objects.
	ID3D11Device*			m_d3dDevice;
	ID3D11DeviceContext*	m_d3dContext;
	IDXGISwapChain1*		m_swapChain;
	ID3D11InputLayout*		m_pInputLayout;

	// Direct3D rendering objects. Required for 3D.
	ID3D11RenderTargetView* m_d3dRenderTargetView;
	ID3D11DepthStencilView* m_d3dDepthStencilView;
	D3D11_VIEWPORT			m_screenViewport;

	ID3D11VertexShader*		m_pVertexShader;
	ID3D11PixelShader*		m_pPixelShader;

	ID3D11InputLayout*		m_pVertexLayout;
	ID3D11Buffer*			m_pVertexBuffer;
	ID3D11Buffer*			m_pIndexBuffer;
	ID3D11Buffer*			m_pConstantBuffer;

	// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
	float					m_effectiveDpi;

	// Cached reference to the Window.
	HWND					m_window;
	bool					dx_active;

	// The IDeviceNotify can be held directly as it owns the DeviceResources.
	IDeviceNotify*			m_deviceNotify;

#ifdef _DEBUG
	// DirectXGI misc
	IDXGIDebug*				m_Debug;
#endif

	// Missi: The DirectX buffer (1/1/2024)
	CQVector<byte>			dxBuffer;
	CQVector<ID2D1Bitmap*>	m_dxBitmaps;
	D3D11_BUFFER_DESC		dxBufferDesc;
	D3D11_SUBRESOURCE_DATA 	dxSubResourceData;

	CDXTexture*				free_dxtextures;
	CDXTexture*				active_dxtextures;

	CDXTexture*				translate_texture;
	CDXTexture*				char_texture;

	byte*					draw_chars;				// 8*8 graphic characters
	CQuakePic*				draw_disc;
	CQuakePic*				draw_backtile;

	cvar_t					dx_nobind {"dx_nobind", "0", false};
	cvar_t					dx_max_size {"dx_max_size", "0", false};
	cvar_t					dx_picmip {"dx_picmip", "0", false};

	cvar_t					dx_fullbrights = { "dx_fullbrights", "0" };
	cvar_t					dx_keeptjunctions = { "dx_keeptjunctions","0" };

	int						dx_hardware_maxsize;

	bool					dx_texture_NPOT;

	int						skytexturenum;
	int						mirrortexturenum;

	POINT*					lightmap_polys[MAX_LIGHTMAPS];
	bool					lightmap_modified[MAX_LIGHTMAPS];
	dxRect_t				lightmap_rectchange[MAX_LIGHTMAPS];
	int						lightmap_count;
	int						allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];	// Missi: changed from a 2D array (12/6/2022)

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte					lightmaps[4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];

	// For gl_texsort 0
	msurface_t*				skychain;
	msurface_t*				waterchain;

	int						lightmap_bytes;		// 1, 2, or 4

	CDXTexture*				lightmap_textures[MAX_LIGHTMAPS];

	unsigned				blocklights[BLOCK_WIDTH * BLOCK_HEIGHT * 3];
	int						active_lightmaps;

	int						last_lightmap_allocated;

	ID3D11Resource*			dxResource;

	int						numdxtextures;

	D3DFORMAT				dx_lightmap_format;
	int						dx_solid_format;
	D3DFORMAT				dx_alpha_format;

	int						d_lightstylevalue[256];

	mleaf_t*				r_viewleaf, *r_oldviewleaf;

	CDXTexture*				particletexture;	// little dot for particles
	CDXTexture*				playertextures[MAX_SCOREBOARD];		// up to 16 color translated skins

	char					q2SkyName[64];
	bool					usesQ2Sky;

	int						r_visframecount;	// bumped when going to a new PVS
	int						r_framecount;		// used for dlight push checking

	CDXRenderer(const CDXRenderer& src);
};

constexpr static size_t MAX_DXTEXTURES = 1024;

extern	entity_t	r_worldentity;
extern	bool		r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t*	currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;

extern float xscaleshrink, yscaleshrink;

extern int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern bool			mirror;
extern mplane_t*	mirror_plane;

extern CDXRenderer* g_pDXRenderer;
extern int vid_width;
extern int vid_height;
extern bool bWantsToRender;

extern cvar_t dx_fullbrights;
extern cvar_t dx_finish;

extern cvar_t	r_norefresh;
extern cvar_t	r_drawentities;
extern cvar_t	r_drawviewmodel;
extern cvar_t	r_speeds;
extern cvar_t	r_fullbright;
extern cvar_t	r_lightmap;
extern cvar_t	r_shadows;
extern cvar_t	r_mirroralpha;
extern cvar_t	r_wateralpha;
extern cvar_t	r_dynamic;
extern cvar_t	r_novis;

extern cvar_t	dx_ztrick;

extern cvar_t	dx_finish;
extern cvar_t	dx_clear;
extern cvar_t	dx_cull;
extern cvar_t	dx_texsort;
extern cvar_t	dx_smoothmodels;
extern cvar_t	dx_affinemodels;
extern cvar_t	dx_polyblend;
extern cvar_t	dx_flashblend;
extern cvar_t	dx_playermip;
extern cvar_t	dx_nocolors;
extern cvar_t	dx_keeptjunctions;
extern cvar_t	dx_reporttjunctions;
extern cvar_t	dx_doubleeyes;
