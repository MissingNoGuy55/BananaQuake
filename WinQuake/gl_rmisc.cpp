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
// r_misc.c

#include "quakedef.h"

/*
==================
R_InitTextures
==================
*/
void	CGLRenderer::R_InitTextures ()
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = g_MemCache->Hunk_AllocName<texture_t>(sizeof(texture_t) + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2, "notexture");
	Q_strcpy(r_notexture_mip->name, "notexture");

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	

	r_notexture_mip->gltexture = GL_LoadTexture(nullptr, "notexture", 16, 16, SRC_INDEXED, (byte*)r_notexture_mip + r_notexture_mip->offsets[0], 0, TEXPREF_PERSIST);
	Con_DPrintf("Initialized checkerboard texture\n");
}

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
void CGLRenderer::R_InitParticleTexture ()
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	particletexture = g_MemCache->Hunk_Alloc<CGLTexture>(sizeof(CGLTexture));
	Q_strncpy(particletexture->identifier, "particle_texture", sizeof(particletexture->identifier));
	particletexture->texnum = texture_extension_number++;
	
	particletexture->width = 128;
	particletexture->height = 128;

	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}

	byte datatest[256] = {};
	memcpy(datatest, data, sizeof(datatest));

	particletexture->pic.datavec.AddMultipleToEnd(sizeof(data), datatest);
	GL_Bind(particletexture);

	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
void CGLRenderer::R_Envmap_f ()
{
	byte*	buffer = g_MemCache->Hunk_Alloc<byte>(256*256*4);	// Missi (3/8/2022)
	char	name[1024];

    memset(name, 0, sizeof(name));
    
	glDrawBuffer  (GL_FRONT);
	glReadBuffer  (GL_FRONT);
	envmap = true;

	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = 256;
	r_refdef.vrect.height = 256;

	r_refdef.viewangles[0] = 0;
	r_refdef.viewangles[1] = 0;
	r_refdef.viewangles[2] = 0;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env0.rgb", buffer, sizeof(buffer));

	r_refdef.viewangles[1] = 90;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env1.rgb", buffer, sizeof(buffer));

	r_refdef.viewangles[1] = 180;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env2.rgb", buffer, sizeof(buffer));

	r_refdef.viewangles[1] = 270;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env3.rgb", buffer, sizeof(buffer));

	r_refdef.viewangles[0] = -90;
	r_refdef.viewangles[1] = 0;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env4.rgb", buffer, sizeof(buffer));

	r_refdef.viewangles[0] = 90;
	r_refdef.viewangles[1] = 0;
	g_GLRenderer->GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	g_GLRenderer->R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	g_Common->COM_WriteFile ("env5.rgb", buffer, sizeof(buffer));

	envmap = false;
	glDrawBuffer  (GL_BACK);
	glReadBuffer  (GL_BACK);
	g_GLRenderer->GL_EndRendering ();
}

/*
===============
R_Init
===============
*/
void CGLRenderer::R_Init ()
{	
	//extern byte *hunk_base;
	extern cvar_t gl_finish;

	g_pCmds->Cmd_AddCommand ("timerefresh", CGLRenderer::R_TimeRefresh_f);
	g_pCmds->Cmd_AddCommand ("envmap", CGLRenderer::R_Envmap_f);
	g_pCmds->Cmd_AddCommand ("pointfile", CCoreRenderer::R_ReadPointFile_f);

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_lightmap);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_shadows);
	Cvar_RegisterVariable (&r_mirroralpha);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_RegisterVariable (&r_dynamic);
	Cvar_RegisterVariable (&r_novis);
	Cvar_RegisterVariable (&r_speeds);

	Cvar_RegisterVariable (&gl_finish);
	Cvar_RegisterVariable (&gl_clear);
	Cvar_RegisterVariable (&gl_texsort);

 	if (gl_mtexable)
		Cvar_SetValue ("gl_texsort", 0.0);

	Cvar_RegisterVariable (&gl_cull);
	Cvar_RegisterVariable (&gl_smoothmodels);
	Cvar_RegisterVariable (&gl_affinemodels);
	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&gl_flashblend);
	Cvar_RegisterVariable (&gl_playermip);
	Cvar_RegisterVariable (&gl_nocolors);

	Cvar_RegisterVariable (&gl_keeptjunctions);
	Cvar_RegisterVariable (&gl_reporttjunctions);

	Cvar_RegisterVariable (&gl_doubleeyes);

	R_InitTextures();

	g_CoreRenderer->R_InitParticles ();
	R_InitParticleTexture ();

#ifdef GLTEST
	Test_Init ();
#endif

	playertextures[0] = new CGLTexture;
	playertextures[0]->texnum = texture_extension_number;
	texture_extension_number += 16;
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void CGLRenderer::R_TranslatePlayerSkin (int playernum)
{
	int		top, bottom;
	int		skinnum;
	byte	translate[256];
	byte*	 pixels;
	char	name[64];
	int			inwidth, inheight;
	int		i;

	model_t	*model;
	aliashdr_t *paliashdr;
	extern	byte		**player_8bit_texels_tbl;

	GL_DisableMultitexture();

	top = cl.scores[playernum].colors & 0xf0;
	bottom = (cl.scores[playernum].colors &15)<<4;

	for (i=0 ; i<256 ; i++)
		translate[i] = i;

	for (i=0 ; i<16 ; i++)
	{
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			translate[TOP_RANGE+i] = top+i;
		else
			translate[TOP_RANGE+i] = top+15-i;
				
		if (bottom < 128)
			translate[BOTTOM_RANGE+i] = bottom+i;
		else
			translate[BOTTOM_RANGE+i] = bottom+15-i;
	}

	//
	// locate the original skin pixels
	//
	currententity = &cl_entities[1+playernum];
	model = currententity->model;
	if (!model)
		return;		// player doesn't have a model yet
	if (model->type != mod_alias)
		return; // only translate skins on alias models

	paliashdr = (aliashdr_t *)Mod_Extradata<aliashdr_t>(model);

	skinnum = currententity->skinnum;

	//TODO: move these tests to the place where skinnum gets received from the server
	if (skinnum < 0 || skinnum >= paliashdr->numskins)
	{
		Con_DPrintf("(%d): Invalid player skin #%d\n", playernum, skinnum);
		skinnum = 0;
}

	pixels = (byte*)paliashdr + paliashdr->texels[skinnum]; // This is not a persistent place!

#if 0
#ifndef WIN64
	s = paliashdr->skinwidth * paliashdr->skinheight;
#else
	s = (long long)paliashdr->skinwidth * (long long)paliashdr->skinheight;
#endif

	if (currententity->skinnum < 0 || currententity->skinnum >= paliashdr->numskins) {
		Con_Printf("(%d): Invalid player skin #%d\n", playernum, currententity->skinnum);
		original = (byte *)paliashdr + paliashdr->texels[0];
	} else
		original = (byte *)paliashdr + paliashdr->texels[currententity->skinnum];
	if (s & 3)
		Sys_Error ("R_TranslateSkin: s&3");
#endif

	inwidth = paliashdr->skinwidth;
	inheight = paliashdr->skinheight;

	snprintf(name, sizeof(name), "player_%i", playernum);
	playertextures[playernum] = GL_LoadTexture(currententity->model, name, paliashdr->skinwidth, paliashdr->skinheight,
		SRC_INDEXED, pixels, paliashdr->gltextures[skinnum][0]->source_offset, TEXPREF_PAD | TEXPREF_OVERWRITE);

	// because this happens during gameplay, do it fast
	// instead of sending it through gl_upload 8
    //g_GLRenderer->GL_Bind(playertextures[playernum]);

/*
#if 0
	byte	translated[320*200];

	for (i=0 ; i<s ; i+=4)
	{
		translated[i] = translate[original[i]];
		translated[i+1] = translate[original[i+1]];
		translated[i+2] = translate[original[i+2]];
		translated[i+3] = translate[original[i+3]];
	}


	// don't mipmap these, because it takes too long
	GL_Upload8 (translated, paliashdr->skinwidth, paliashdr->skinheight, false, false, true);
#else
	scaled_width = gl_max_size.value < 512 ? gl_max_size.value : 512;
	scaled_height = gl_max_size.value < 256 ? gl_max_size.value : 256;

	// allow users to crunch sizes down even more if they want
	scaled_width >>= (int)gl_playermip.value;
	scaled_height >>= (int)gl_playermip.value;

	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];

	out = pixels;
	fracstep = inwidth*0x10000/scaled_width;
	for (i=0 ; i<scaled_height ; i++, out += scaled_width)
	{
		inrow = original + inwidth*(i*inheight/scaled_height);
		frac = fracstep >> 1;
		for (j=0 ; j<scaled_width ; j+=4)
		{
			out[j] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+1] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+2] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+3] = translate32[inrow[frac>>16]];
			frac += fracstep;
		}
	}
	glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
*/

}


/*
===============
R_NewMap
===============
*/
void CGLRenderer::R_NewMap ()
{
	int		i;

	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	/*
	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;
	*/

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;
	g_CoreRenderer->R_ClearParticles ();

	GL_BuildLightmaps ();

	for (const char* levelData = cl.worldmodel->entities; levelData; levelData = g_Common->COM_ParseStringNewline(levelData))
	{
		if (!Q_strncmp(levelData, "}", 1))
			break;
        if (!Q_strncmp(levelData, "\"sky\"", 5) || !Q_strncmp(levelData, "\"skyname\"", 9))
		{
			levelData = g_Common->COM_ParseStringNewline(levelData);
			int len = 1;

			while (levelData[++len] != '\"')
			{
			}

			Q_strncpy(q2SkyName, levelData+1, len-1);

			usesQ2Sky = true;
			break;
		}

        // Missi: HACK: Crossfire in HL1 has no skybox set by default, which defaults to 'desert' in HL1, so we have to do this (7/19/2024)
        if (!Q_strncmp(cl.worldmodel->name, "maps/crossfire.bsp", 18))
        {
            Q_strncpy(q2SkyName, "desert", 6);

            usesQ2Sky = true;
            break;
        }
	}

	if (usesQ2Sky)
		Con_Printf("Level uses Quake 2-styled skies\n");
	else
		Con_Printf("Level uses Quake 1-styled skies\n");

	// identify sky texture
	skytexturenum = -1;
	mirrortexturenum = -1;
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		if (!cl.worldmodel->textures[i])
			continue;
		if (!Q_strncmp(cl.worldmodel->textures[i]->name,"sky",3) )
			skytexturenum = i;
		if (!Q_strncmp(cl.worldmodel->textures[i]->name,"window02_1",10) )
			mirrortexturenum = i;
 		cl.worldmodel->textures[i]->texturechain = NULL;
	}

    if (usesQ2Sky)
        R_LoadSkys ();
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void CGLRenderer::R_TimeRefresh_f ()
{
	int			i;
	float		start, stop, time;

	glDrawBuffer  (GL_FRONT);
	glFinish ();

	start = Sys_DoubleTime ();
	for (i=0 ; i<128 ; i++)
	{
		r_refdef.viewangles[1] = i/128.0*360.0;
		g_GLRenderer->R_RenderView ();
	}

	glFinish ();
	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);

	glDrawBuffer  (GL_BACK);
	g_GLRenderer->GL_EndRendering ();
}

void D_FlushCaches ()
{
}


