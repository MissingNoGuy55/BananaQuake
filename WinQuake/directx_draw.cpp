#include "quakedef.h"

static byte	conback_buffer[sizeof(CQuakePic)];
CQuakePic* conback = (CQuakePic*)&conback_buffer;

int		texture_extension_number = 1;

static constexpr short MAX_SCRAPS = 2;

static int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
static byte			scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT * 4];
static bool			scrap_dirty;
static CDXTexture*	scrap_textures[MAX_SCRAPS];
static int			scrap_texnum;

static int			scrap_uploads;

CDXTexture*			currenttexture = NULL;		// to avoid unnecessary texture sets

unsigned int d_8to24table_fbright[256];
unsigned int d_8to24table_fbright_fence[256];
unsigned int d_8to24table_nobright[256];
unsigned int d_8to24table_nobright_fence[256];
unsigned int d_8to24table_conchars[256];
unsigned int d_8to24table_shirt[256];
unsigned int d_8to24table_pants[256];

cvar_t	r_norefresh = { "r_norefresh","0" };
cvar_t	r_drawentities = { "r_drawentities","1" };
cvar_t	r_drawviewmodel = { "r_drawviewmodel","1" };
cvar_t	r_speeds = { "r_speeds","0" };
cvar_t	r_fullbright = { "r_fullbright","0" };
cvar_t	r_lightmap = { "r_lightmap","0" };
cvar_t	r_shadows = { "r_shadows","0" };
cvar_t	r_mirroralpha = { "r_mirroralpha","1" };
cvar_t	r_wateralpha = { "r_wateralpha","1" };
cvar_t	r_dynamic = { "r_dynamic","1" };
cvar_t	r_novis = { "r_novis","0" };

byte	dottexture[8][8] =
{
	{0,1,1,0,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{0,1,1,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

unsigned short	d_8to16table[256];
unsigned int	d_8to24table[256];
void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

texture_s* r_notexture_mip;
tagRECT window_rect;
cvar_t _windowed_mouse	{ "_windowed_mouse", "1", false };
cvar_t scr_viewsize		{ "scr_viewsize", "0", false };
cvar_t dx_fullbrights	{ "dx_fullbrights", "0", false };
cvar_t dx_finish		{ "dx_finish", "0", false };

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CDXRenderer::CompileShaderFromFile(LPCWCH szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

/*
===============
R_Init
===============
*/
void CDXRenderer::R_Init(void)
{
	//extern byte *hunk_base;
	extern cvar_t dx_finish;

	//Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	//Cmd_AddCommand ("envmap", R_Envmap_f);
	g_pCmds->Cmd_AddCommand("pointfile", CCoreRenderer::R_ReadPointFile_f);

	Cvar_RegisterVariable(&r_norefresh);
	Cvar_RegisterVariable(&r_lightmap);
	Cvar_RegisterVariable(&r_fullbright);
	Cvar_RegisterVariable(&r_drawentities);
	Cvar_RegisterVariable(&r_drawviewmodel);
	Cvar_RegisterVariable(&r_shadows);
	Cvar_RegisterVariable(&r_mirroralpha);
	Cvar_RegisterVariable(&r_wateralpha);
	Cvar_RegisterVariable(&r_dynamic);
	Cvar_RegisterVariable(&r_novis);
	Cvar_RegisterVariable(&r_speeds);

	Cvar_RegisterVariable(&dx_finish);

	g_CoreRenderer->R_InitParticles();
	R_InitParticleTexture();

#ifdef GLTEST
	Test_Init();
#endif

	playertextures[0] = new CDXTexture;
	playertextures[0]->texnum = texture_extension_number;
	texture_extension_number += 16;
}

void CDXRenderer::R_InitParticleTexture(void)
{
	int		x, y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	particletexture = g_MemCache->Hunk_Alloc<CDXTexture>(sizeof(CDXTexture));
	particletexture->texnum = texture_extension_number++;
	DX_Bind(particletexture);

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y] * 255;
		}
	}
	/*glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
}

/*
===============
CGLRenderer::Draw_Init
===============
*/
void CDXRenderer::Draw_Init (void)
{
	char	ver[40];
	int		start = 0;
	uintptr_t offset = 0;
	int		i = 0;

	Cvar_RegisterVariable (&dx_nobind);
	Cvar_RegisterVariable (&dx_max_size);
	Cvar_RegisterVariable (&dx_picmip);

    /*Cvar_RegisterVariable (&level_fog_color_r);
    Cvar_RegisterVariable (&level_fog_color_g);
    Cvar_RegisterVariable (&level_fog_color_b);
    Cvar_RegisterVariable (&level_fog_color_goal_r);
    Cvar_RegisterVariable (&level_fog_color_goal_g);
    Cvar_RegisterVariable (&level_fog_color_goal_b);
    Cvar_RegisterVariable (&level_fog_density);
    Cvar_RegisterVariable (&level_fog_density_goal);
    Cvar_RegisterVariable (&level_fog_start);
    Cvar_RegisterVariable (&level_fog_start_goal);
    Cvar_RegisterVariable (&level_fog_end);
    Cvar_RegisterVariable (&level_fog_end_goal);
    Cvar_RegisterVariable (&level_fog_lerp_time);
    Cvar_RegisterVariable (&level_fog_force);*/

	memset(ver, 0, sizeof(ver));

	draw_chars = W_GetLumpName<byte>("conchars");

	if (!draw_chars)
	{
		uintptr_t path;
		draw_chars = COM_LoadHunkFile<byte>("gfx/conchars.lmp", &path);
	}

	offset = (uintptr_t)draw_chars - (uintptr_t)wad_base;

	// Missi: FIXME: for some reason, this needs to be done as the changes from QuakeSpasm
	// for applying d_8to24table_conchars will make the charset blank (9/13/2023)
	for (i = 0; i < 256 * 64; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	char_texture = DX_LoadTexture (NULL, "charset", 128, 128, SRC_INDEXED, draw_chars, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);

	start = g_MemCache->Hunk_LowMark();

	/*glTexParameterf(DX_TEXTURE_2D, DX_TEXTURE_MIN_FILTER, DX_NEAREST);
	glTexParameterf(DX_TEXTURE_2D, DX_TEXTURE_MAG_FILTER, DX_NEAREST);*/

	conback = Draw_CachePic("gfx/conback.lmp");
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	// free loaded console
	g_MemCache->Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	translate_texture = g_MemCache->Hunk_AllocName<CDXTexture>(sizeof(CDXTexture), "dummy");
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
CGLRenderer::GL_FindTexture
================
*/
CDXTexture* CDXRenderer::DX_FindTexture(model_t* model, const char* identifier)
{
	CDXTexture* glt;

	if (identifier)
	{
		for (glt = active_dxtextures; glt; glt = glt->next)
		{
			if (glt->owner == model && !strcmp(glt->identifier, identifier))
				return glt;
		}
	}

	return NULL;
}

/*
================
CGLRenderer::DX_LoadTexture -- Missi: revised (3/18/2022)

Loads an OpenGL texture via string ID or byte data. Can take an empty string if you don't need a name.
================
*/
CDXTexture* CDXRenderer::DX_LoadTexture(model_t* owner, const char* identifier, int width, int height, enum srcformat_t format, byte* data, uintptr_t offset, int flags)
{
	CDXTexture* glt = nullptr;
	int				CRCBlock = 0;
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

	if ((flags & TEXPREF_OVERWRITE) && (glt = DX_FindTexture(owner, identifier)))
	{
		if (glt->checksum == (unsigned int)CRCBlock)
			return glt;
	}
	else
		glt = DX_NewTexture();

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
		DX_LoadImage8(glt, data);
		break;
	case SRC_LIGHTMAP:
		DX_LoadLightmap(glt, data);
		break;
	case SRC_RGBA:
		DX_LoadImage32(glt, (unsigned*)data);
		break;
	default:
		return nullptr;
	}

	texture_extension_number++;

	g_MemCache->Hunk_FreeToLowMark(mark);

	return glt;
}

CDXTexture* CDXRenderer::DX_NewTexture()
{
	CDXTexture* glt = NULL;

	D3D11_TEXTURE3D_DESC desc;
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	ID3D11Texture3D* pTexture = NULL;
	GetD3DDevice()->CreateTexture3D(&desc, NULL, &pTexture);

	if (numdxtextures == MAX_DXTEXTURES)
		Sys_Error("numgltextures == MAX_GLTEXTURES\n");

	glt = free_dxtextures;
	free_dxtextures = glt->next;
	glt->next = active_dxtextures;
	active_dxtextures = glt;

	numdxtextures++;
	return glt;
}

unsigned* CDXRenderer::DX_8to32(byte* in, int pixels, unsigned int* usepal)
{
	int i;
	unsigned* out, * data;

	out = data = g_MemCache->Hunk_Alloc<unsigned>(pixels * 4);

	for (i = 0; i < pixels; i++)
		*out++ = usepal[*in++];

	return data;
}

/*
================
CGLRenderer::GL_MipMapW
================
*/
unsigned* CDXRenderer::DX_MipMapW(unsigned* data, int width, int height)
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

unsigned* CDXRenderer::DX_MipMapH(unsigned* data, int width, int height)
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
CGLRenderer::DX_ResampleTexture
================
*/
unsigned* CDXRenderer::DX_ResampleTexture(unsigned* in, int inwidth, int inheight, bool alpha)
{
	byte* nwpx, * nepx, * swpx, * sepx, * dest;
	unsigned xfrac, yfrac, x, y, modx, mody, imodx, imody, injump, outjump;
	unsigned* out;
	int i, j, outwidth, outheight;

	if (inwidth == DX_Pad(inwidth) && inheight == DX_Pad(inheight))
		return in;

	outwidth = DX_Pad(inwidth);
	outheight = DX_Pad(inheight);
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
CGLRenderer::DX_Resample8BitTexture -- JACK
================
*/
void CDXRenderer::DX_Resample8BitTexture(byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight)
{
	int		i, j;
	unsigned	char* inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth * 0x10000 / outwidth;
	for (i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth * (i * inheight / outheight);
		frac = fracstep >> 1;
		for (j = 0; j < outwidth; j += 4)
		{
			out[j] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 1] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 2] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 3] = inrow[frac >> 16];
			frac += fracstep;
		}
	}
}

/*
===============
CGLRenderer::GL_AlphaEdgeFix

eliminate pink edges on sprites, etc.
operates in place on 32bit data
===============
*/
void CDXRenderer::DX_AlphaEdgeFix(byte* data, int width, int height)
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

void CDXRenderer::DX_Bind(CDXTexture* tex)
{
	if (dx_nobind.value)
		tex = char_texture;
	if (currenttexture == tex)
		return;
	currenttexture = tex;

	UINT mips = D3D11CalcSubresource(0, 0, 4);

	GetD3DDeviceContext()->UpdateSubresource(dxResource, mips, NULL, tex->pic.datavec.GetBase(), (tex->source_width / 4) * (tex->source_height / 4), 32);
}

/*
================
CGLRenderer::DX_LoadImage32 -- handles 32bit source data
================
*/
void CDXRenderer::DX_LoadImage32(CDXTexture* glt, unsigned* data)
{
	int	internalformat, miplevel, mipwidth, mipheight, picmip;

	if (!dx_texture_NPOT)
	{
		// resample up
		data = DX_ResampleTexture(data, glt->width, glt->height, glt->flags & TEXPREF_ALPHA);
		glt->width = DX_Pad(glt->width);
		glt->height = DX_Pad(glt->height);
	}

	// mipmap down
	picmip = (glt->flags & TEXPREF_NOPICMIP) ? 0 : q_max((int)dx_picmip.value, 0);
	mipwidth = DX_SafeTextureSize(glt->width >> picmip);
	mipheight = DX_SafeTextureSize(glt->height >> picmip);
	while ((int)glt->width > mipwidth)
	{
		DX_MipMapW(data, glt->width, glt->height);
		glt->width >>= 1;
		if (glt->flags & TEXPREF_ALPHA)
			DX_AlphaEdgeFix((byte*)data, glt->width, glt->height);
	}
	while ((int)glt->height > mipheight)
	{
		DX_MipMapH(data, glt->width, glt->height);
		glt->height >>= 1;
		if (glt->flags & TEXPREF_ALPHA)
			DX_AlphaEdgeFix((byte*)data, glt->width, glt->height);
	}

	// upload
	DX_Bind(glt);
	internalformat = (glt->flags & TEXPREF_ALPHA) ? dx_alpha_format : dx_solid_format;
	glt->pic.datavec.AddMultipleToEnd(glt->source_width * glt->source_height, (byte*)data);

	D2D1_SIZE_U             size;

	size.width = glt->width;
	size.height = glt->height;

	D2D1_BITMAP_PROPERTIES bmpProperties = {};
	bmpProperties.dpiX = 96;
	bmpProperties.dpiY = 96;
	bmpProperties.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bmpProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED;

	ID2D1Bitmap* D2DBitmap = {};

	UINT32 pitch = 32;

	HRESULT res = GetD2DDeviceContext()->CreateBitmap(size, data, NULL, &bmpProperties, &D2DBitmap);

	THROW_IF_FAIL(res);

	m_dxBitmaps.AddToEnd(D2DBitmap);

	//glTexImage2D(DX_TEXTURE_2D, 0, internalformat, glt->width, glt->height, 0, DX_RGBA, DX_UNSIGNED_BYTE, data);

	// upload mipmaps
	if (glt->flags & TEXPREF_MIPMAP && !(glt->flags & TEXPREF_WARPIMAGE)) // warp image mipmaps are generated later
	{
		mipwidth = glt->width;
		mipheight = glt->height;

		for (miplevel = 1; mipwidth > 1 || mipheight > 1; miplevel++)
		{
			if (mipwidth > 1)
			{
				DX_MipMapW(data, mipwidth, mipheight);
				mipwidth >>= 1;
			}
			if (mipheight > 1)
			{
				DX_MipMapH(data, mipwidth, mipheight);
				mipheight >>= 1;
			}
			//glTexImage2D(DX_TEXTURE_2D, miplevel, internalformat, mipwidth, mipheight, 0, DX_RGBA, DX_UNSIGNED_BYTE, data);
		}
	}

	// set filter modes
	//DX_SetFilterModes(glt);
}

/*
================
CGLRenderer::GL_Pad -- return smallest power of two greater than or equal to s
================
*/
int CDXRenderer::DX_Pad(int s)
{
	int i;
	for (i = 1; i < s; i <<= 1)
		;
	return i;
}

/*
===============
CGLRenderer::DX_SafeTextureSize -- return a size with hardware and user prefs in mind
===============
*/
int CDXRenderer::DX_SafeTextureSize(int s)
{
	if (!dx_texture_NPOT)
		s = DX_Pad(s);
	if ((int)dx_max_size.value > 0)
		s = q_min(DX_Pad((int)dx_max_size.value), s);
	s = q_min(dx_hardware_maxsize, s);
	return s;
}

/*
================
CGLRenderer::GL_PadImageW -- return image with width padded up to power-of-two dimentions
================
*/
byte* CDXRenderer::DX_PadImageW(byte* in, int width, int height, byte padbyte)
{
	int i, j, outwidth;
	byte* out, * data;

	if (width == DX_Pad(width))
		return in;

	outwidth = DX_Pad(width);

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
byte* CDXRenderer::DX_PadImageH(byte* in, int width, int height, byte padbyte)
{
	int i, srcpix, dstpix;
	byte* data, * out;

	if (height == DX_Pad(height))
		return in;

	srcpix = width * height;
	dstpix = width * DX_Pad(height);

	out = data = g_MemCache->Hunk_Alloc<byte>(dstpix);

	for (i = 0; i < srcpix; i++)
		*out++ = *in++;
	for (; i < dstpix; i++)
		*out++ = padbyte;

	return data;
}

/*
================
CGLRenderer::GL_PadConditional -- only pad if a texture of that size would be padded. (used for tex coords)
================
*/
int CDXRenderer::DX_PadConditional(int s)
{
	if (s < DX_SafeTextureSize(s))
		return DX_Pad(s);
	else
		return s;
}

/*
===============
CGLRenderer::GL_PadEdgeFixW -- special case of AlphaEdgeFix for textures that only need it because they were padded

operates in place on 32bit data, and expects unpadded height and width values
===============
*/
void CDXRenderer::DX_PadEdgeFixW(byte* data, int width, int height)
{
	byte* src, * dst;
	int i, padw, padh;

	padw = DX_PadConditional(width);
	padh = DX_PadConditional(height);

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
void CDXRenderer::DX_PadEdgeFixH(byte* data, int width, int height)
{
	byte* src, * dst;
	int i, padw, padh;

	padw = DX_PadConditional(width);
	padh = DX_PadConditional(height);

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
CGLRenderer::DX_LoadImage8 -- handles 8bit source data, then passes it to LoadImage32
================
*/
void CDXRenderer::DX_LoadImage8(CDXTexture* glt, byte* data)
{
	extern cvar_t dx_fullbrights;
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
	else if (glt->flags & TEXPREF_NOBRIGHT && dx_fullbrights.value)
	{
		if (glt->flags & TEXPREF_ALPHA)
			usepal = d_8to24table_nobright_fence;
		else
			usepal = d_8to24table_nobright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_CONCHARS)
	{
		// Missi: FIXME: for some reason, this needs to be set to d_8to24table as the changes from QuakeSpasm
		// for applying d_8to24table_conchars will make the charset blank (9/13/2023)
		usepal = d_8to24table;
		padbyte = 255;
	}
	else
	{
		usepal = d_8to24table;
		padbyte = 255;
	}

	// pad each dimention, but only if it's not going to be downsampled later
	if (glt->flags & TEXPREF_PAD)
	{
		if ((int)glt->width < DX_SafeTextureSize(glt->width))
		{
			data = DX_PadImageW(data, glt->width, glt->height, padbyte);
			glt->width = DX_Pad(glt->width);
			padw = true;
		}
		if ((int)glt->height < DX_SafeTextureSize(glt->height))
		{
			data = DX_PadImageH(data, glt->width, glt->height, padbyte);
			glt->height = DX_Pad(glt->height);
			padh = true;
		}
	}

	// convert to 32bit
	data = (byte*)DX_8to32(data, glt->width * glt->height, usepal);

	// fix edges
	if (glt->flags & TEXPREF_ALPHA)
		DX_AlphaEdgeFix(data, glt->width, glt->height);
	else
	{
		if (padw)
			DX_PadEdgeFixW(data, glt->source_width, glt->source_height);
		if (padh)
			DX_PadEdgeFixH(data, glt->source_width, glt->source_height);
	}

	// upload it
	DX_LoadImage32(glt, (unsigned*)data);
}

/*
================
CGLRenderer::DX_LoadLightmap -- handles lightmap data
================
*/
void CDXRenderer::DX_LoadLightmap(CDXTexture* glt, byte* data)
{
	// upload it
	DX_Bind(glt);
	
	
	//glTexImage2D(GL_TEXTURE_2D, 0, lightmap_bytes, glt->width, glt->height, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, data);

	//// set filter modes
	//DX_SetFilterModes(glt);
}

void CDXRenderer::Scrap_Upload(void)
{
	char	name[8];
	int		i;

	scrap_uploads++;

	for (i = 0; i < MAX_SCRAPS; i++)
	{
		snprintf(name, sizeof(name), "scrap%i", i);
		scrap_textures[i] = DX_LoadTexture(NULL, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_INDEXED, scrap_texels[i],
			(src_offset_t)scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
	}
	scrap_dirty = false;
}

void D_FlushCaches(void)
{
}
