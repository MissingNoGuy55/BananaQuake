#include "quakedef.h"

/*
===============
R_NewMap
===============
*/
void CDXRenderer::R_NewMap(void)
{
	int		i;

	for (i = 0; i < 256; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	/*
	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;
	*/

	// clear out efrags in case the level hasn't been reloaded
	// FIXME: is this one short?
	for (i = 0; i < cl.worldmodel->numleafs; i++)
		cl.worldmodel->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	g_CoreRenderer->R_ClearParticles();

	DX_BuildLightmaps();

	for (const char* levelData = cl.worldmodel->entities; levelData; levelData = g_Common->COM_ParseStringNewline(levelData))
	{
		if (!Q_strncmp(levelData, "}", 1))
			break;
		if (!Q_strncmp(levelData, "\"sky\"", 3))
		{
			levelData = g_Common->COM_ParseStringNewline(levelData);
			int len = 1;

			while (levelData[++len] != '\"')
			{
			}

			Q_strncpy(q2SkyName, levelData + 1, len - 1);

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
	for (i = 0; i < cl.worldmodel->numtextures; i++)
	{
		if (!cl.worldmodel->textures[i])
			continue;
		if (!Q_strncmp(cl.worldmodel->textures[i]->name, "sky", 3))
			skytexturenum = i;
		if (!Q_strncmp(cl.worldmodel->textures[i]->name, "window02_1", 10))
			mirrortexturenum = i;
		cl.worldmodel->textures[i]->texturechain = NULL;
	}
#if 0
	if (usesQ2Sky)
		R_LoadSkys();
#endif
}