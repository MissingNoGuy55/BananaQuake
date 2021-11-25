#pragma once

int R_LightPoint(vec3_t p);
void R_MarkLights(dlight_t* light, int bit, mnode_t* node);
void R_AnimateLight(void);
void R_RenderDlights(void);