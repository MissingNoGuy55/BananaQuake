
#include "quakedef.h"

extern GLuint gl_bmodel_vbo;

/*
==================
GL_BuildBModelVertexBuffer

Deletes gl_bmodel_vbo if it already exists, then rebuilds it with all
surfaces from world + all brush models
==================
*/
void CGLRenderer::GL_BuildBModelVertexBuffer(void)
{
	unsigned int	numverts, varray_bytes, varray_index;
	int		i, j;
	model_t* m;
	float* varray;

	if (!(gl_vbo_able && gl_mtexable && gl_max_texture_units >= 3))
		return;

	// ask GL for a name for our VBO
	GL_DeleteBuffersFunc(1, &gl_bmodel_vbo);
	GL_GenBuffersFunc(1, &gl_bmodel_vbo);

	// count all verts in all models
	numverts = 0;
	for (j = 1; j < MAX_MODELS; j++)
	{
		m = cl.model_precache[j];
		if (!m || m->name[0] == '*' || m->type != mod_brush)
			continue;

		for (i = 0; i < m->numsurfaces; i++)
		{
			numverts += m->surfaces[i].numedges;
		}
	}

	// build vertex array
	varray_bytes = VERTEXSIZE * sizeof(float) * numverts;
	varray = (float*)malloc(varray_bytes);
	varray_index = 0;

	for (j = 1; j < MAX_MODELS; j++)
	{
		m = cl.model_precache[j];
		if (!m || m->name[0] == '*' || m->type != mod_brush)
			continue;

		for (i = 0; i < m->numsurfaces; i++)
		{
			msurface_t* s = &m->surfaces[i];
			s->vbo_firstvert = varray_index;
			memcpy(&varray[VERTEXSIZE * varray_index], s->polys->verts, VERTEXSIZE * sizeof(float) * s->numedges);
			varray_index += s->numedges;
		}
	}

	// upload to GPU
	GL_BindBufferFunc(GL_ARRAY_BUFFER, gl_bmodel_vbo);
	GL_BufferDataFunc(GL_ARRAY_BUFFER, varray_bytes, varray, GL_STATIC_DRAW);
	free(varray);

	// invalidate the cached bindings
	GL_ClearBufferBindings();
}
