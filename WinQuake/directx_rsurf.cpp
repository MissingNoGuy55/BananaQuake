#include "quakedef.h"

mvertex_t* r_pcurrentvertbase;
model_t* currentmodel;

int	nColinElim;

/*
===============
R_AddDynamicLights
===============
*/
void CDXRenderer::R_AddDynamicLights(msurface_t* surf)
{
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t* tex;

	float		cred, cgreen, cblue, brightness;
	unsigned* bl;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	tex = surf->texinfo;

	for (lnum = 0; lnum < MAX_DLIGHTS; lnum++)
	{
		if (!(surf->dlightbits[lnum >> 5] & (1U << (lnum & 31))))
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) -
			surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i = 0; i < 3; i++)
		{
			impact[i] = cl_dlights[lnum].origin[i] -
				surf->plane->normal[i] * dist;
		}

		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		// Missi: copied from QuakeSpasm (6/7/2024)
		bl = blocklights;
		cred = cl_dlights[lnum].color[0] * 256.0f;
		cgreen = cl_dlights[lnum].color[1] * 256.0f;
		cblue = cl_dlights[lnum].color[2] * 256.0f;
		//
		for (t = 0; t < tmax; t++)
		{
			td = local[1] - t * 16;
			if (td < 0)
				td = -td;
			for (s = 0; s < smax; s++)
			{
				sd = local[0] - s * 16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);
				if (dist < minlight)
					// Missi: copied from QuakeSpasm (6/7/2024)
				{
					brightness = rad - dist;
					bl[0] += (int)(brightness * cred);
					bl[1] += (int)(brightness * cgreen);
					bl[2] += (int)(brightness * cblue);
				}
				bl += 3;
				//
			}
		}

		/*for (t = 0; t<tmax; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
					blocklights[t*smax + s] += (rad - dist)*256;
			}
		}*/
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void CDXRenderer::R_BuildLightMap(msurface_t* surf, byte* dest, int stride)
{

	int			smax, tmax;
	int			t;
	int			i, j, size;
	unsigned	r, g, b;
	int			maps;
	unsigned	scale;
	int			lightadj[4];
	unsigned* bl;

	byte* lightmap;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax;
	lightmap = surf->samples;

	if (cl.worldmodel->lightdata)
	{
		// set to full bright if no light data
		memset(&blocklights[0], 0, size * 3 * sizeof(unsigned int)); // Missi: copied from QuakeSpasm (6/7/2024)

		// clear to no light
		for (i = 0; i < size; i++)
			blocklights[i] = 0;

		// add all the lightmaps
		if (lightmap)
			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
				maps++)
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			// Missi: copied from QuakeSpasm (6/7/2024)
			bl = blocklights;
			for (i = 0; i < size; i++)
			{
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
			}
			//
		}

		// add all the dynamic lights
		if (surf->dlightframe == r_framecount)
			R_AddDynamicLights(surf);
	}
	else
	{
		// set to full bright if no light data
		memset(&blocklights[0], 255, size * 3 * sizeof(unsigned int)); // Missi: copied from QuakeSpasm (6/7/2024)
	}

	// bound, invert, and shift
	switch (dx_lightmap_format)
	{
	case D3DFMT_A8R8G8B8:
		stride -= smax * 4;
		bl = blocklights;
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				r = *bl++ >> 7;
				g = *bl++ >> 7;
				b = *bl++ >> 7;

				*dest++ = (r > 255) ? 255 : r;
				*dest++ = (g > 255) ? 255 : g;
				*dest++ = (b > 255) ? 255 : b;
				*dest++ = 255;
			}
		}
		break;
	case D3DFMT_X8B8G8R8:
		stride -= smax * 4;
		bl = blocklights;
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				r = *bl++ >> 7;
				g = *bl++ >> 7;
				b = *bl++ >> 7;

				*dest++ = (b > 255) ? 255 : b;
				*dest++ = (g > 255) ? 255 : g;
				*dest++ = (r > 255) ? 255 : r;
				*dest++ = 255;
			}
		}
		break;
	default:
		Sys_Error("R_BuildLightMap: bad lightmap format");
	}
}

/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void CDXRenderer::DX_BuildLightmaps(void)
{
	int		i = 0, j = 0;
	model_t* m = NULL;

	//memset (allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	//Missi: copied from QuakeSpasm (6/3/2024)
	memset(allocated, 0, sizeof(allocated));
	memset(blocklights, 0, sizeof(blocklights));
	memset(lightmaps, 0, sizeof(lightmaps));
	memset(lightmap_modified, 0, sizeof(lightmap_modified));
	memset(lightmap_polys, 0, sizeof(lightmap_polys));
	memset(lightmap_rectchange, 0, sizeof(lightmap_rectchange));

	memset(cl_dlights, 0, sizeof(cl_dlights));
	memset(cl_lightstyle, 0, sizeof(*lightmap_rectchange));

	last_lightmap_allocated = 0;
	active_lightmaps = 0;
	lightmap_count = 0;
	lightmap_bytes = 0;
	memset(lightmap_textures, 0, sizeof(lightmap_textures));

	dx_lightmap_format = D3DFMT_R8G8B8;
	// default differently on the Permedia
	/*if (isPermedia)
		gl_lightmap_format = GL_RGBA;*/

	/*if (g_Common->COM_CheckParm("-lm_1"))
		gl_lightmap_format = GL_LUMINANCE;
	if (g_Common->COM_CheckParm("-lm_a"))
		gl_lightmap_format = GL_ALPHA;
	if (g_Common->COM_CheckParm("-lm_i"))
		gl_lightmap_format = GL_INTENSITY;
	if (g_Common->COM_CheckParm("-lm_2"))
		gl_lightmap_format = GL_RGBA4;
	if (g_Common->COM_CheckParm("-lm_4"))
		gl_lightmap_format = GL_RGBA;*/

	switch (dx_lightmap_format)
	{
	case D3DFMT_R8G8B8:
		lightmap_bytes = 4;
		break;
	case D3DFMT_X4R4G4B4:
		lightmap_bytes = 2;
		break;
	case D3DFMT_A1:
	/*case GL_INTENSITY:
	case GL_ALPHA:*/
		lightmap_bytes = 1;
		break;
	}

	for (j = 1; j < MAX_MODELS; j++)
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i = 0; i < m->numsurfaces; i++)
		{
			DX_CreateSurfaceLightmap(m->surfaces + i);
			if (m->surfaces[i].flags & SURF_DRAWTURB)
				continue;
#ifndef QUAKE2
			if (m->surfaces[i].flags & SURF_DRAWSKY)
				continue;
#endif
			BuildSurfaceDisplayList(m->surfaces + i);
		}
	}

	//
	// upload all lightmaps that were filled
	//
	for (i = 0; i < MAX_LIGHTMAPS; i++, lightmap_count++)
	{
		char name[64];
		snprintf(name, sizeof(name), "lightmap%07i", i);

		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].right = (unsigned char)BLOCK_WIDTH;
		lightmap_rectchange[i].top = (unsigned char)BLOCK_HEIGHT;
		lightmap_rectchange[i].left = 0;
		lightmap_rectchange[i].bottom = 0;

		lightmap_textures[i] = DX_LoadTexture(cl.worldmodel, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_LIGHTMAP, lightmaps + (i * BLOCK_WIDTH * BLOCK_HEIGHT) * lightmap_bytes, (src_offset_t)lightmaps + (i * BLOCK_WIDTH * BLOCK_HEIGHT) * lightmap_bytes, TEXPREF_LINEAR | TEXPREF_NOPICMIP);
	}
}

/*
================
BuildSurfaceDisplayList
================
*/
void CDXRenderer::BuildSurfaceDisplayList(msurface_t* fa)
{
	int			i, lindex, lnumverts, s_axis, t_axis;
	float		dist, lastdist, lzi, scale, u, v, frac;
	unsigned	mask;
	float* vec;
	vec3_t		local, transformed;
	medge_t* pedges, * r_pedge;
	mplane_t* pplane;
	int			vertpage, newverts, newpage, lastvert;
	bool	visible;
	float		s, t;
	dxpoly_t* poly;

	// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	//
	// draw texture
	//
	// Missi: do it right here or you risk messing up the entire renderer (11/27/2022)


	poly = g_MemCache->Hunk_Alloc<dxpoly_t>(sizeof(dxpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!dx_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER))
	{
		for (i = 0; i < lnumverts; ++i)
		{
			vec3_t v1, v2;
			float* prev, * fthis, * next;
			float f;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			fthis = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract(fthis, prev, v1);
			VectorNormalize(v1);
			VectorSubtract(next, prev, v2);
			VectorNormalize(v2);

			// skip co-linear points
#define COLINEAR_EPSILON 0.001
			if ((fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
				(fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) &&
				(fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;

}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int CDXRenderer::AllocBlock(int w, int h, int* x, int* y)
{
	int		i, j;
	int		best, best2;
	int		bestx;
	int		texnum;

	for (texnum = 0; texnum < MAX_LIGHTMAPS; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++)
		{
			best2 = 0;

			for (j = 0; j < w; j++)
			{
				if (allocated[texnum][i + j] >= best)
					break;
				if (allocated[texnum][i + j] > best2)
					best2 = allocated[texnum][i + j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i = 0; i < w; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error("AllocBlock: full");
	return 0;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void CDXRenderer::DX_CreateSurfaceLightmap(msurface_t* surf)
{
	int		smax, tmax, s, t, l, i;
	byte* base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum = AllocBlock(smax, tmax, &surf->light_s, &surf->light_t);

	base = lightmaps + surf->lightmaptexturenum * lightmap_bytes * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;
	R_BuildLightMap(surf, base, BLOCK_WIDTH * lightmap_bytes);
}