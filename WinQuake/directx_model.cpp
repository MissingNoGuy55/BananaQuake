#include "quakedef.h"

model_t* loadmodel = {};
char loadname[32] = {};
byte mod_novis[MAX_MAP_LEAFS / 8] = {};
model_t	mod_known[MAX_MOD_KNOWN] = {};
int		mod_numknown = 0;

void CDXRenderer::DX_DeleteTexture(CDXTexture* kill)
{

}

/*
================
CGLRenderer::GL_FreeTexture
================
*/
void CDXRenderer::DX_FreeTexture(CDXTexture* kill)
{
	CDXTexture* glt, * next;

	if (kill == NULL)
	{
		Con_Printf("GL_FreeTexture: NULL texture\n");
		return;
	}

	if (active_dxtextures == kill)
	{
		active_dxtextures = kill->next;
		kill->next = free_dxtextures;
		free_dxtextures = kill;

		DX_DeleteTexture(kill);
		numdxtextures--;
		return;
	}

	for (glt = active_dxtextures; glt; glt = glt->next)
	{
		next = glt->next;

		if (glt->next == kill)
		{
			glt->next = kill->next;
			kill->next = free_dxtextures;
			free_dxtextures = kill;

			DX_DeleteTexture(kill);
			numdxtextures--;
			return;
		}
	}

	Con_Printf("GL_FreeTexture: not found\n");
}

void CDXRenderer::DX_FreeTextureForModel(model_t* mod)
{
	CDXTexture* glt = nullptr, * next = nullptr;

	for (glt = active_dxtextures; glt; glt = next)
	{
		next = glt->next;
		if (glt && glt->owner == mod)
			DX_FreeTexture(glt);
	}
}

/*
===============
Mod_Init
===============
*/
void Mod_Init(void)
{
	Cvar_RegisterVariable(&gl_subdivide_size);
	memset(mod_novis, 0xff, sizeof(mod_novis));
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model)
{
	mnode_t* node;
	float		d;
	mplane_t* plane;

	if (!model || !model->nodes)
		Sys_Error("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte* Mod_DecompressVis(byte* in, model_t* model)
{
	static byte	decompressed[MAX_MAP_LEAFS / 8];
	int		c;
	byte* out;
	int		row;

	row = (model->numleafs + 7) >> 3;
	out = decompressed;

#if 0
	memcpy(out, in, row);
#else
	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

byte* Mod_LeafPVS(mleaf_t* leaf, model_t* model)
{
	if (leaf == model->leafs)
		return mod_novis;
	return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll(void)
{
	int			i = 0;
	model_t* mod = nullptr;

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
		if (mod->type != mod_alias)
		{
			mod->needload = true;

			if (cls.state != ca_dedicated)
				g_pDXRenderer->DX_FreeTextureForModel(mod);
		}
}

/*
==================
Mod_FindName

==================
*/
model_t* Mod_FindName(const char* name)
{
	int		i;
	model_t* mod;

	if (!name[0])
		Sys_Error("Mod_ForName: NULL name");

	//
	// search the currently loaded models
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
		if (!strcmp(mod->name, name))
			break;

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			Sys_Error("mod_numknown == MAX_MOD_KNOWN");
		Q_strcpy(mod->name, name);
		mod->needload = true;
		mod_numknown++;
	}

	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(const char* name)
{
	model_t* mod;

	mod = Mod_FindName(name);

	if (!mod->needload)
	{
		if (mod->type == mod_alias)
			g_MemCache->Cache_Check<model_t>(&mod->cache); // Missi: come back to this later (11/22/2022)
	}
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t* Mod_ForName(const char* name, bool crash)
{
	model_t* mod;

	mod = Mod_FindName(name);

	return Mod_LoadModel<model_t>(mod, crash);
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte* mod_base;

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
template<typename T>
T* Mod_LoadSpriteFrame (T* pin, mspriteframe_t **ppframe, int framenum)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					i, width, height, size, origin[2];
	unsigned short		*ppixout;
	byte				*ppixin;
	char				name[64];
	uintptr_t			offset;

	pinframe = (dspriteframe_t *)pin;

	offset = (uintptr_t)pinframe + 1 - (uintptr_t)mod_base;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;

	pspriteframe = g_MemCache->Hunk_AllocName<mspriteframe_t>(sizeof(mspriteframe_t), loadname);

	Q_memset (pspriteframe, 0, sizeof (mspriteframe_t));

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	if (cls.state != ca_dedicated)
	{
		sprintf(name, "%s_%i", loadmodel->name, framenum);
		pspriteframe->gltexture = g_pDXRenderer->DX_LoadTexture(loadmodel, name, width, height, SRC_INDEXED, (byte*)(pinframe + 1), offset, TEXPREF_ALPHA | TEXPREF_MIPMAP);
	}

	return (T *)((byte *)pinframe + sizeof (dspriteframe_t) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
template<typename T>
T * Mod_LoadSpriteGroup (void * pin, mspriteframe_t **ppframe, int framenum)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	T					*ptemp;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	pspritegroup = g_MemCache->Hunk_AllocName<mspritegroup_t>(sizeof (mspritegroup_t) +
				(numframes - 1) * sizeof (pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = g_MemCache->Hunk_AllocName<float>(numframes * sizeof (float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error ("Mod_LoadSpriteGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (T*)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame<T>(ptemp, &pspritegroup->frames[i], framenum * 100 + i);
	}

	return (T*)ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	int					i;
	int					version;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;
	
	pin = (dsprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
		Sys_Error ("%s has wrong version number "
				 "(%i should be %i)", mod->name, version, SPRITE_VERSION);

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = g_MemCache->Hunk_AllocName<msprite_t>(size, loadname);

	mod->cache.data = (byte*)psprite;

	psprite->type = LittleLong (pin->type);
	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = static_cast<synctype_t>(LittleLong (pin->synctype));
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	
//
// load the frames
//
	if (numframes < 1)
		Sys_Error ("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

	mod->numframes = numframes;

	pframetype = (dspriteframetype_t *)(pin + 1);

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = static_cast<spriteframetype_t>(LittleLong (pframetype->type));
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = Mod_LoadSpriteFrame<dspriteframetype_t>(pframetype + 1,
										 &psprite->frames[i].frameptr, i);
		}
		else
		{
			pframetype = Mod_LoadSpriteGroup<dspriteframetype_t>(pframetype + 1,
										 &psprite->frames[i].frameptr, i);
		}
	}

	mod->type = mod_sprite;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length(corner);
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures(lump_t* l)
{
	if (cls.state == ca_dedicated)
		return;

	int		i, j, pixels, num, max, altmax;
	miptex_t* mt;
	texture_t* tx, * tx2;
	texture_t* anims[10];
	texture_t* altanims[10];
	uintptr_t		offset;
	dmiptexlump_t* m;

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}
	m = (dmiptexlump_t*)(mod_base + l->fileofs);

	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = g_MemCache->Hunk_AllocName<texture_t*>(m->nummiptex * sizeof(*loadmodel->textures), loadname);

	for (i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t*)((byte*)m + m->dataofs[i]);
		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		if ((mt->width & 15) || (mt->height & 15))
			Sys_Error("Texture %s is not 16 aligned", mt->name);
		pixels = mt->width * mt->height / 64 * 85;
		tx = g_MemCache->Hunk_AllocName<texture_t>(sizeof(texture_t) + pixels, loadname);
		loadmodel->textures[i] = tx;

		memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (j = 0; j < MIPLEVELS; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
		// the pixels immediately follow the structures
		memcpy(tx + 1, mt + 1, pixels);

		g_pDXRenderer->SetUsesQuake2Skybox(false);

		if (!Q_strncmp(mt->name, "sky", 3))
			g_pDXRenderer->R_InitSky(tx);
		else
		{
			offset = (src_offset_t)(mt + 1) - (src_offset_t)mod_base;

			//texture_mode = GL_LINEAR_MIPMAP_NEAREST; //_LINEAR;
			tx->gltexture = g_pDXRenderer->DX_LoadTexture(loadmodel, mt->name, tx->width, tx->height, SRC_INDEXED, (byte*)(tx + 1), offset, TEXPREF_NONE);
			//texture_mode = GL_LINEAR;
		}
	}

	//
	// sequence the animations
	//
	for (i = 0; i < m->nummiptex; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// allready sequenced

		// find the number of frames in the animation
		memset(anims, 0, sizeof(anims));
		memset(altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error("Bad animating texture %s", tx->name);

		for (j = i + 1; j < m->nummiptex; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp(tx2->name + 2, tx->name + 2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num + 1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num + 1 > altmax)
					altmax = num + 1;
			}
			else
				Sys_Error("Bad animating texture %s", tx->name);
		}

#define	ANIM_CYCLE	2
		// link them all together
		for (j = 0; j < max; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error("Missing frame %i of %s", j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j = 0; j < altmax; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error("Missing frame %i of %s", j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_LoadLighting

Missi: some stuff from QuakeSpasm ported to here, mostly just for .lit support (6/7/2024)
=================
*/
void Mod_LoadLighting(lump_t* l)
{
	int i, mark;
	byte* in, * out, * data;
	byte d, q64_b0, q64_b1;
	char litfilename[MAX_OSPATH];
	uintptr_t path_id;

	loadmodel->lightdata = NULL;
	// LordHavoc: check for a .lit file
	Q_strlcpy(litfilename, loadmodel->name, sizeof(litfilename));
	g_Common->COM_StripExtension(litfilename, litfilename);
	Q_strlcat(litfilename, ".lit", sizeof(litfilename));
	mark = g_MemCache->Hunk_LowMark();

	data = COM_LoadHunkFile<byte>(litfilename, &path_id);
	if (data)
	{
		// use lit file only from the same gamedir as the map
		// itself or from a searchpath with higher priority.
		if (path_id < loadmodel->path_id)
		{
			g_MemCache->Hunk_FreeToLowMark(mark);
			Con_DPrintf("ignored %s from a gamedir with lower priority\n", litfilename);
		}
		else
			if (data[0] == 'Q' && data[1] == 'L' && data[2] == 'I' && data[3] == 'T')
			{
				i = LittleLong(((int*)data)[1]);
				if (i == 1)
				{
					if (8 + l->filelen * 3 == g_Common->com_filesize)
					{
						Con_DPrintf("%s loaded\n", litfilename);
						loadmodel->lightdata = data + 8;
						return;
					}
					g_MemCache->Hunk_FreeToLowMark(mark);
					Con_Printf("Outdated .lit file (%s should be %u bytes, not %u)\n", litfilename, 8 + l->filelen * 3, g_Common->com_filesize);
				}
				else
				{
					g_MemCache->Hunk_FreeToLowMark(mark);
					Con_Printf("Unknown .lit file version (%d)\n", i);
				}
			}
			else
			{
				g_MemCache->Hunk_FreeToLowMark(mark);
				Con_Printf("Corrupt .lit file (old version?), ignoring\n");
			}
	}
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}

#ifdef OLD_LIGHTING
	loadmodel->lightdata = g_MemCache->Hunk_AllocName<byte>(l->filelen, loadname);
	memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
#else
	loadmodel->lightdata = (byte*)g_MemCache->Hunk_AllocName<byte>(l->filelen * 3, litfilename);
	in = loadmodel->lightdata + l->filelen * 2; // place the file at the end, so it will not be overwritten until the very last write
	out = loadmodel->lightdata;
	memcpy(in, mod_base + l->fileofs, l->filelen);
	for (i = 0; i < l->filelen; i++)
	{
		d = *in++;
		*out++ = d;
		*out++ = d;
		*out++ = d;
	}
#endif
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = g_MemCache->Hunk_AllocName<byte>(l->filelen, loadname);
	memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = g_MemCache->Hunk_AllocName<char>(l->filelen, loadname);
	memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes(lump_t* l)
{
	dvertex_t* in;
	mvertex_t* out;
	int			i, count;

	in = reinterpret_cast<dvertex_t*>((mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = g_MemCache->Hunk_AllocName<mvertex_t>(count * sizeof(*out), loadname);

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels(lump_t* l)
{
	dmodel_t* in;
	dmodel_t* out;
	int			i, j, count;

	in = reinterpret_cast<dmodel_t*>((mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = g_MemCache->Hunk_AllocName<dmodel_t>(count * sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (j = 0; j < MAX_MAP_HULLS; j++)
			out->headnode[j] = LittleLong(in->headnode[j]);
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges(lump_t* l, int bsp2)
{
	dedge_t* in;
	medge_t* out;
	int 	i, count;

	if (bsp2 > 0)
	{
		dledge_t* in = (dledge_t*)(mod_base + l->fileofs);

		if (l->filelen % sizeof(*in))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

		count = l->filelen / sizeof(*in);
		out = (medge_t*)g_MemCache->Hunk_AllocName<medge_t>((count + 1) * sizeof(*out), loadname);

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for (i = 0; i < count; i++, in++, out++)
		{
			out->v[0] = LittleLong(in->v[0]);
			out->v[1] = LittleLong(in->v[1]);
		}
	}
	else
	{
		in = reinterpret_cast<dedge_t*>((mod_base + l->fileofs));
		if (l->filelen % sizeof(*in))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*in);
		out = g_MemCache->Hunk_AllocName<medge_t>((count + 1) * sizeof(*out), loadname);

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for (i = 0; i < count; i++, in++, out++)
		{
			out->v[0] = (unsigned short)LittleShort(in->v[0]);
			out->v[1] = (unsigned short)LittleShort(in->v[1]);
		}
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo(lump_t* l)
{
	texinfo_t* in;
	mtexinfo_t* out;
	int 	i, j, count;
	int		miptex;
	float	len1, len2;

	in = reinterpret_cast<texinfo_t*>((mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = g_MemCache->Hunk_AllocName<mtexinfo_t>(count * sizeof(*out), loadname);

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 4; j++)       // Missi: -Waggressive-loop-optimizations fix (6/19/2024)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}
		len1 = Length(out->vecs[0]);
		len2 = Length(out->vecs[1]);
		len1 = (len1 + len2) / 2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;

		miptex = LittleLong(in->miptex);
		out->flags = LittleLong(in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(msurface_t* s)
{
	float	mins[2], maxs[2], val;
	int		i, j, e;
	mvertex_t* v;
	mtexinfo_t* tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = Q_MAXFLOAT;
	maxs[0] = maxs[1] = -Q_MAXFLOAT;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++)
		{
			/* The following calculation is sensitive to floating-point
			 * precision.  It needs to produce the same result that the
			 * light compiler does, because R_BuildLightMap uses surf->
			 * extents to know the width/height of a surface's lightmap,
			 * and incorrect rounding here manifests itself as patches
			 * of "corrupted" looking lightmaps.
			 * Most light compilers are win32 executables, so they use
			 * x87 floating point.  This means the multiplies and adds
			 * are done at 80-bit precision, and the result is rounded
			 * down to 32-bits and stored in val.
			 * Adding the casts to double seems to be good enough to fix
			 * lighting glitches when Quakespasm is compiled as x86_64
			 * and using SSE2 floating-point.  A potential trouble spot
			 * is the hallway at the beginning of mfxsp17.  -- Missi: copied from QuakeSpasm (12/4/2022)
			 */
			val = ((double)v->position[0] * (double)tex->vecs[j][0]) +
				((double)v->position[1] * (double)tex->vecs[j][1]) +
				((double)v->position[2] * (double)tex->vecs[j][2]) +
				(double)tex->vecs[j][3];

			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

		if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 2000) // was 512 in glquake, 256 in winquake -- Missi: copied from QuakeSpasm (6/3/2024)
			Sys_Error("Bad surface extents");
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces(lump_t* l, int bsp2)
{
	dsface_t* ins;
	dlface_t* inl;
	msurface_t* out;
	int			i, count, surfnum, lofs;
	int			planenum, side, texinfon;

	if (bsp2)
	{
		ins = NULL;
		inl = (dlface_t*)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = (dsface_t*)(mod_base + l->fileofs);
		inl = NULL;
		if (l->filelen % sizeof(*ins))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*ins);
	}

	out = (msurface_t*)g_MemCache->Hunk_AllocName<msurface_t>(count * sizeof(*out), loadname);

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, out++)
	{
		if (bsp2)
		{
			out->firstedge = LittleLong(inl->firstedge);
			out->numedges = LittleLong(inl->numedges);
			planenum = LittleLong(inl->planenum);
			side = LittleLong(inl->side);
			texinfon = LittleLong(inl->texinfo);
			for (i = 0; i < MAXLIGHTMAPS; i++)
				out->styles[i] = inl->styles[i];
			lofs = LittleLong(inl->lightofs);
			inl++;
		}
		else
		{
			out->firstedge = LittleLong(ins->firstedge);
			out->numedges = LittleShort(ins->numedges);
			planenum = LittleShort(ins->planenum);
			side = LittleShort(ins->side);
			texinfon = LittleShort(ins->texinfo);
			for (i = 0; i < MAXLIGHTMAPS; i++)
				out->styles[i] = ins->styles[i];
			lofs = LittleLong(ins->lightofs);
			ins++;
		}

		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + texinfon;

		CalcSurfaceExtents(out);

		// lighting info
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + (lofs * 3);

		// set the drawing flags flag

		if (cls.state != ca_dedicated)
		{
			if (!Q_strncmp(out->texinfo->texture->name, "sky", 3))	// sky
			{
				out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);

				if (!g_pDXRenderer->UsesQuake2Skybox())
					g_pDXRenderer->DX_SubdivideSurface(out);	// cut up polygon for warps
				continue;
			}

			if (!Q_strncmp(out->texinfo->texture->name, "*", 1))		// turbulent
			{
				out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
				for (i = 0; i < 2; i++)
				{
					out->extents[i] = 16384;
					out->texturemins[i] = -8192;
				}
				g_pDXRenderer->DX_SubdivideSurface(out);	// cut up polygon for warps
				continue;
			}
		}
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent(mnode_t* node, mnode_t* parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

static void Mod_LoadNodes_L1(lump_t* l)
{
	int			i, j, count, p;
	dl1node_t* in;
	mnode_t* out;

	in = (dl1node_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("Mod_LoadNodes: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t*)g_MemCache->Hunk_AllocName<mnode_t>(count * sizeof(*out), loadname);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleLong(in->firstface);
		out->numsurfaces = LittleLong(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p >= 0 && p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 0xffffffff - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p >= 0 && p < loadmodel->numleafs)
					out->children[j] = (mnode_t*)(loadmodel->leafs + p);
				else
				{
					Con_Printf("Mod_LoadNodes: invalid leaf index %i (file has only %i leafs)\n", p, loadmodel->numleafs);
					out->children[j] = (mnode_t*)(loadmodel->leafs); //map it to the solid leaf
				}
			}
		}
	}
}

static void Mod_LoadNodes_L2(lump_t* l)
{
	int			i, j, count, p;
	dl2node_t* in;
	mnode_t* out;

	in = (dl2node_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("Mod_LoadNodes: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t*)g_MemCache->Hunk_AllocName<mnode_t>(count * sizeof(*out), loadname);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleLong(in->firstface);
		out->numsurfaces = LittleLong(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p > 0 && p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 0xffffffff - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p >= 0 && p < loadmodel->numleafs)
					out->children[j] = (mnode_t*)(loadmodel->leafs + p);
				else
				{
					Con_Printf("Mod_LoadNodes: invalid leaf index %i (file has only %i leafs)\n", p, loadmodel->numleafs);
					out->children[j] = (mnode_t*)(loadmodel->leafs); //map it to the solid leaf
				}
			}
		}
	}
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes(lump_t* l, int bsp2)
{
	if (bsp2 == 2)
		Mod_LoadNodes_L2(l);
	else if (bsp2)
		Mod_LoadNodes_L1(l);
	else
	{
		int			i, j, count, p;
		dnode_t* in;
		mnode_t* out;

		in = static_cast<dnode_t*>(static_cast<void*>((mod_base + l->fileofs)));
		if (l->filelen % sizeof(*in))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*in);
		out = g_MemCache->Hunk_AllocName<mnode_t>(count * sizeof(*out), loadname);

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->minmaxs[j] = LittleShort(in->mins[j]);
				out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
			}

			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleShort(in->firstface);
			out->numsurfaces = LittleShort(in->numfaces);

			for (j = 0; j < 2; j++)
			{
				p = LittleShort(in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
			}
		}

	}
	Mod_SetParent(loadmodel->nodes, NULL);	// sets nodes and leafs
}

static void Mod_ProcessLeafs_L1(dl1leaf_t* in, int filelen)
{
	mleaf_t* out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Sys_Error("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);

	count = filelen / sizeof(*in);

	out = (mleaf_t*)g_MemCache->Hunk_AllocName<mleaf_t>(count * sizeof(*out), loadname);

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces + LittleLong(in->firstmarksurface);
		out->nummarksurfaces = LittleLong(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j = 0; j < 4; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

	}
}

static void Mod_ProcessLeafs_L2(dl2leaf_t* in, int filelen)
{
	mleaf_t* out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Sys_Error("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);

	count = filelen / sizeof(*in);

	out = (mleaf_t*)g_MemCache->Hunk_AllocName<mleaf_t>(count * sizeof(*out), loadname);

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces + LittleLong(in->firstmarksurface);
		out->nummarksurfaces = LittleLong(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j = 0; j < 4; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(lump_t* l, int bsp2)
{
	void* data = (void*)(mod_base + l->fileofs);

	if (bsp2 == 2)
		Mod_ProcessLeafs_L2((dl2leaf_t*)data, l->filelen);
	else if (bsp2)
		Mod_ProcessLeafs_L1((dl1leaf_t*)data, l->filelen);
	else
	{
		dleaf_t* in;
		mleaf_t* out;
		int			i, j, count, p;

		in = reinterpret_cast<dleaf_t*>((mod_base + l->fileofs));
		if (l->filelen % sizeof(*in))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*in);
		out = g_MemCache->Hunk_AllocName<mleaf_t>(count * sizeof(*out), loadname);

		loadmodel->leafs = out;
		loadmodel->numleafs = count;

		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->minmaxs[j] = LittleShort(in->mins[j]);
				out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces +
				LittleShort(in->firstmarksurface);
			out->nummarksurfaces = LittleShort(in->nummarksurfaces);

			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			out->efrags = NULL;

			for (j = 0; j < 4; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

			// Missi: ifdef'd out because it shows seams, reveals secrets and is otherwise ugly.
			// it also tends to not work with modern compilers very well unless you specifically
			// code one to.
			// 
			// if you really need it, remove this ifdef or define OLD_WATER_WARP in CMakeLists.txt
#ifdef OLD_WATER_WARP
			// gl underwater warp
			if (out->contents != CONTENTS_EMPTY)
			{
				for (j = 0; j < out->nummarksurfaces; j++)
					out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
			}
#endif
		}
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes(lump_t* l, int bsp2)
{
	dclipnode_t* ins = nullptr;
	dlclipnode_t* inl = nullptr;
	mclipnode_t* out;
	int			i, count;
	hull_t* hull;

	if (bsp2)
	{
		ins = NULL;
		inl = (dlclipnode_t*)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
			Sys_Error("Mod_LoadClipnodes: funny lump size in %s", loadmodel->name);

		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = reinterpret_cast<dclipnode_t*>((mod_base + l->fileofs));
		if (l->filelen % sizeof(*ins))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*ins);
	}
	out = (mclipnode_t*)g_MemCache->Hunk_AllocName<mclipnode_t>(count * sizeof(*out), loadname);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	if (bsp2)
	{
		for (i = 0; i < count; i++, out++, inl++)
		{
			out->planenum = LittleLong(inl->planenum);

			if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
				host->Host_Error("Mod_LoadClipnodes: planenum out of bounds");

			out->children[0] = LittleLong(inl->children[0]);
			out->children[1] = LittleLong(inl->children[1]);
			//Spike: FIXME: bounds check
		}
	}
	else
	{
		for (i = 0; i < count; i++, out++, ins++)
		{
			out->planenum = LittleLong(ins->planenum);
			out->children[0] = LittleShort(ins->children[0]);
			out->children[1] = LittleShort(ins->children[1]);
		}
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0(void)
{
	mnode_t* in, * child;
	mclipnode_t* out;
	int			i, j, count;
	hull_t* hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = g_MemCache->Hunk_AllocName<mclipnode_t>(count * sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j = 0; j < 2; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(lump_t* l, int bsp2)
{
	int		i, j, count;
	short* in;
	msurface_t** out;

	if (bsp2)
	{
		unsigned int* in = (unsigned int*)(mod_base + l->fileofs);

		if (l->filelen % sizeof(*in))
			host->Host_Error("Mod_LoadMarksurfaces: funny lump size in %s", loadmodel->name);

		count = l->filelen / sizeof(*in);
		out = (msurface_t**)g_MemCache->Hunk_AllocName<msurface_t*>(count * sizeof(*out), loadname);

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for (i = 0; i < count; i++)
		{
			j = LittleLong(in[i]);
			if (j >= loadmodel->numsurfaces)
				host->Host_Error("Mod_LoadMarksurfaces: bad surface number");
			out[i] = loadmodel->surfaces + j;
		}
	}
	else
	{
		in = reinterpret_cast<short*>((mod_base + l->fileofs));
		if (l->filelen % sizeof(*in))
			Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
		count = l->filelen / sizeof(*in);
		out = g_MemCache->Hunk_AllocName<msurface_t*>(count * sizeof(*out), loadname);

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for (i = 0; i < count; i++)
		{
			j = (unsigned short)LittleShort(in[i]);
			if (j >= loadmodel->numsurfaces)
				Sys_Error("Mod_LoadMarksurfaces: bad surface number");
			out[i] = loadmodel->surfaces + j;
		}
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(lump_t* l)
{
	int		i, count;
	int* in, * out;

	in = reinterpret_cast<int*>((mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = g_MemCache->Hunk_AllocName<int>(count * sizeof(*out), loadname);

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes(lump_t* l)
{
	int			i, j;
	mplane_t* out;
	dplane_t* in;
	int			count;
	int			bits;

	in = reinterpret_cast<dplane_t*>((mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = g_MemCache->Hunk_AllocName<mplane_t>(count * 2 * sizeof(*out), loadname);

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel(model_t* mod, void* buffer)
{
	int			i = 0, j = 0;
	int			bsp2 = 0;
	dheader_t* header = nullptr;
	dmodel_t* bm = nullptr;
	float		radius = 0.0f;

	loadmodel->type = mod_brush;

	header = (dheader_t*)buffer;

	mod->bspversion = LittleLong(header->version);

	switch (mod->bspversion)
	{
	case BSPVERSION:
		bsp2 = false;
		break;
	case BSP2VERSION_2PSB:
		bsp2 = 1;	//first iteration
		break;
	case BSP2VERSION_BSP2:
		bsp2 = 2;	//sanitised revision
		break;
	case BSPVERSION_QUAKE64:
		bsp2 = false;
		break;
	default:
		Sys_Error("Mod_LoadBrushModel: %s has unsupported version number (%i)", mod->name, mod->bspversion);
		break;
	}

	// swap all the lumps
	mod_base = (byte*)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

	// load into heap

	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES], bsp2);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
	if (cls.state != ca_dedicated)
	{
		Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
		Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);	// Missi: not needed on servers, but it causes some caching issues I think (6/8/2024)
	}
	Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[LUMP_FACES], bsp2);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES], bsp2);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS], bsp2);
	Mod_LoadNodes(&header->lumps[LUMP_NODES], bsp2);
	Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES], bsp2);
	Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0();

	mod->numframes = 2;		// regular and alternate animation

	//
	// set up the submodels (FIXME: this is confusing)
	//
	for (i = 0; i < mod->numsubmodels; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j = 1; j < MAX_MAP_HULLS; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy(bm->maxs, mod->maxs);
		VectorCopy(bm->mins, mod->mins);

		radius = RadiusFromBounds(mod->mins, mod->maxs);
		mod->rmaxs[0] = mod->rmaxs[1] = mod->rmaxs[2] = mod->ymaxs[0] = mod->ymaxs[1] = mod->ymaxs[2] = radius;
		mod->rmins[0] = mod->rmins[1] = mod->rmins[2] = mod->ymins[0] = mod->ymins[1] = mod->ymins[2] = -radius;

		if (i > 0 || strcmp(mod->name, sv->modelname) != 0) //skip submodel 0 of sv.worldmodel, which is the actual world
		{
			// start with the hull0 bounds
			VectorCopy(mod->maxs, mod->clipmaxs);
			VectorCopy(mod->mins, mod->clipmins);

			// process hull1 (we don't need to process hull2 becuase there's
			// no such thing as a brush that appears in hull2 but not hull1)
			//Mod_BoundsFromClipNode (mod, 1, mod->hulls[1].firstclipnode); // (disabled for now becuase it fucks up on rotating models)
		}

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{	// duplicate the basic information
			char	name[12];

			sprintf(name, "*%i", i + 1);
			loadmodel = Mod_FindName(name);
			*loadmodel = *mod;
			strcpy(loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s\n",mod->cache.data, mod->name);
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

aliashdr_t* pheader;

stvert_t	stverts[MAXALIASVERTS];
mtriangle_t	triangles[MAXALIASTRIS];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
trivertx_t* poseverts[MAXALIASFRAMES];
int			posenum;

byte** player_8bit_texels_tbl;
byte* player_8bit_texels;

/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

extern unsigned int d_8to24table[];

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void Mod_FloodFillSkin(byte* skin, int skinwidth, int skinheight)
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte* pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP(-1, -1, 0);
		if (x < skinwidth - 1)	FLOODFILL_STEP(1, 1, 0);
		if (y > 0)				FLOODFILL_STEP(-skinwidth, 0, -1);
		if (y < skinheight - 1)	FLOODFILL_STEP(skinwidth, 0, 1);
		skin[x + skinwidth * y] = fdc;
	}
}

/*
===============
Mod_LoadAllSkins
===============
*/
template<typename T>
T* Mod_LoadAllSkins(int numskins, daliasskintype_t* pskintype)
{
	int		i, j, k;
	char	name[32];
	int		s;
	byte* copy;
	byte* skin;
	byte* texels;
	daliasskingroup_t* pinskingroup;
	int		groupskins;
	daliasskininterval_t* pinskinintervals;
	uintptr_t	offset;

	offset = (uintptr_t)pskintype + 1 - (uintptr_t)mod_base;

	skin = (byte*)(pskintype + 1);

	if (numskins < 1 || numskins > MAX_SKINS)
		Sys_Error("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

	s = pheader->skinwidth * pheader->skinheight;

	for (i = 0; i < numskins; i++)
	{
		if (pskintype->type == ALIAS_SKIN_SINGLE) {
			Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);

			// save 8 bit texels for the player model to remap
	//		if (!strcmp(loadmodel->name,"progs/player.mdl")) {
			texels = g_MemCache->Hunk_AllocName<byte>(s, loadname);
			pheader->texels[i] = texels - (byte*)pheader;
			memcpy(texels, (byte*)(pskintype + 1), s);
			//		}

			if (cls.state != ca_dedicated)
			{
				sprintf(name, "%s_%i", loadmodel->name, i);
				pheader->dxtextures[i][0] =
					pheader->dxtextures[i][1] =
					pheader->dxtextures[i][2] =
					pheader->dxtextures[i][3] =
					g_pDXRenderer->DX_LoadTexture(loadmodel, name, pheader->skinwidth,
						pheader->skinheight, SRC_INDEXED, (byte*)(pskintype + 1), offset, TEXPREF_MIPMAP);
			}
			pskintype = (daliasskintype_t*)((byte*)(pskintype + 1) + s);
		}
		else {
			// animating skin group.  yuck.
			pskintype++;
			pinskingroup = (daliasskingroup_t*)pskintype;
			groupskins = LittleLong(pinskingroup->numskins);
			pinskinintervals = (daliasskininterval_t*)(pinskingroup + 1);

			pskintype = reinterpret_cast<daliasskintype_t*>((pinskinintervals + groupskins));

			for (j = 0; j < groupskins; j++)
			{
				Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);
				if (j == 0) {
					texels = g_MemCache->Hunk_AllocName<byte>(s, loadname);
					pheader->texels[i] = texels - (byte*)pheader;
					memcpy(texels, (byte*)(pskintype), s);
				}
				sprintf(name, "%s_%i_%i", loadmodel->name, i, j);
				if (cls.state != ca_dedicated)
				{
					pheader->dxtextures[i][j & 3] =
						g_pDXRenderer->DX_LoadTexture(loadmodel, name, pheader->skinwidth,
							pheader->skinheight, SRC_INDEXED, (byte*)(pskintype), offset, TEXPREF_MIPMAP);
				}
				pskintype = (daliasskintype_t*)((byte*)(pskintype)+s);
			}
			k = j;
			for (/* */; j < 4; j++)
				pheader->dxtextures[i][j & 3] =
				pheader->dxtextures[i][j - k];
		}
	}

	return (T*)pskintype;
}

//=========================================================================

/*
=================
Mod_LoadAliasFrame
=================
*/
void* Mod_LoadAliasFrame(void* pin, maliasframedesc_t* frame)
{
	trivertx_t* pframe = nullptr, * pinframe = nullptr;
	int				i = 0;
	daliasframe_t* pdaliasframe;

	pdaliasframe = (daliasframe_t*)pin;

	Q_strcpy(frame->name, pdaliasframe->name);
	frame->firstpose = posenum;
	frame->numposes = 1;

	for (i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmin.v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertx_t*)(pdaliasframe + 1);

	poseverts[posenum] = pinframe;
	posenum++;

	pinframe += pheader->numverts;

	return (void*)pinframe;
}

//=========================================================

/*
=================
Mod_LoadAliasGroup
=================
*/
void* Mod_LoadAliasGroup(void* pin, maliasframedesc_t* frame)
{
	daliasgroup_t* pingroup;
	int					i, numframes;
	daliasinterval_t* pin_intervals;
	void* ptemp;

	pingroup = (daliasgroup_t*)pin;

	numframes = LittleLong(pingroup->numframes);

	frame->firstpose = posenum;
	frame->numposes = numframes;

	for (i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmin.v[i] = pingroup->bboxmax.v[i];
	}

	pin_intervals = (daliasinterval_t*)(pingroup + 1);

	frame->interval = LittleFloat(pin_intervals->interval);

	pin_intervals += numframes;

	ptemp = (void*)pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		poseverts[posenum] = (trivertx_t*)((daliasframe_t*)ptemp + 1);
		posenum++;

		ptemp = (trivertx_t*)((daliasframe_t*)ptemp + 1) + pheader->numverts;
	}

	return ptemp;
}

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(model_t* mod, void* buffer)
{
	int					i, j;
	mdl_t* pinmodel;
	stvert_t* pinstverts;
	dtriangle_t* pintriangles;
	int					version, numframes, numskins;
	int					size;
	daliasframetype_t* pframetype;
	daliasskintype_t* pskintype;
	int					start, end, total;

	start = g_MemCache->Hunk_LowMark();

	pinmodel = (mdl_t*)buffer;

	version = LittleLong(pinmodel->version);
	if (version != ALIAS_VERSION)
		Sys_Error("%s has wrong version number (%i should be %i)",
			mod->name, version, ALIAS_VERSION);

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	size = sizeof(aliashdr_t)
		+ (LittleLong(pinmodel->numframes) - 1) *
		sizeof(pheader->frames[0]);
	pheader = g_MemCache->Hunk_AllocName<aliashdr_t>(size, loadname);

	mod->flags = LittleLong(pinmodel->flags);

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pheader->boundingradius = LittleFloat(pinmodel->boundingradius);
	pheader->numskins = LittleLong(pinmodel->numskins);
	pheader->skinwidth = LittleLong(pinmodel->skinwidth);
	pheader->skinheight = LittleLong(pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Sys_Error("model %s has a skin taller than %d", mod->name,
			MAX_LBM_HEIGHT);

	pheader->numverts = LittleLong(pinmodel->numverts);

	if (pheader->numverts <= 0)
		Sys_Error("model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS)
		Sys_Error("model %s has too many vertices", mod->name);

	pheader->numtris = LittleLong(pinmodel->numtris);

	if (pheader->numtris <= 0)
		Sys_Error("model %s has no triangles", mod->name);

	pheader->numframes = LittleLong(pinmodel->numframes);
	numframes = pheader->numframes;
	if (numframes < 1)
		Sys_Error("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pheader->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = static_cast<synctype_t>(LittleLong(pinmodel->synctype));
	mod->numframes = pheader->numframes;

	for (i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat(pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}


	//
	// load the skins
	//
	pskintype = (daliasskintype_t*)&pinmodel[1];
	pskintype = Mod_LoadAllSkins<daliasskintype_t>(pheader->numskins, pskintype);

	//
	// load base s and t vertices
	//
	pinstverts = (stvert_t*)pskintype;

	for (i = 0; i < pheader->numverts; i++)
	{
		stverts[i].onseam = LittleLong(pinstverts[i].onseam);
		stverts[i].s = LittleLong(pinstverts[i].s);
		stverts[i].t = LittleLong(pinstverts[i].t);
	}

	//
	// load triangle lists
	//
	pintriangles = (dtriangle_t*)&pinstverts[pheader->numverts];

	for (i = 0; i < pheader->numtris; i++)
	{
		triangles[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (j = 0; j < 3; j++)
		{
			triangles[i].vertindex[j] =
				LittleLong(pintriangles[i].vertindex[j]);
		}
	}

	//
	// load the frames
	//
	posenum = 0;
	pframetype = (daliasframetype_t*)&pintriangles[pheader->numtris];

	for (i = 0; i < numframes; i++)
	{
		aliasframetype_t	frametype;

		frametype = (aliasframetype_t)LittleLong(pframetype->type);

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)
				Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
		}
		else
		{
			pframetype = (daliasframetype_t*)
				Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);
		}
	}

	pheader->numposes = posenum;

	mod->type = mod_alias;

	// FIXME: do this right

	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

	//
	// build the draw lists
	//
	DX_MakeAliasModelDisplayLists(mod, pheader);

	//
	// move the complete, relocatable alias model to the cache
	//	
	end = g_MemCache->Hunk_LowMark();
	total = end - start;

	g_MemCache->Cache_Alloc<void>(&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	memcpy(mod->cache.data, pheader, total);

	g_MemCache->Hunk_FreeToLowMark(start);
}

//=============================================================================
