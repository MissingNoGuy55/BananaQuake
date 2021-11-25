#pragma once

void BoundPoly(int numverts, float* verts, vec3_t mins, vec3_t maxs);

void SubdividePolygon(int numverts, float* verts);

void GL_SubdivideSurface(msurface_t* fa);

void EmitWaterPolys(msurface_t* fa);

void EmitSkyPolys(msurface_t* fa);

void EmitBothSkyLayers(msurface_t* fa);

void R_DrawSkyChain(msurface_t* s);

void R_InitSky(texture_t* mt);