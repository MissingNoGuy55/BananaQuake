#pragma once

#ifndef GLRSURF_H
#define GLRSURF_H

void R_DrawBrushModel(entity_t* e);
void R_DrawWorld(void);
void R_DrawWaterSurfaces(void);
void R_RenderBrushPoly(msurface_t* fa);
void GL_BuildLightmaps(void);

#endif