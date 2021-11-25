#pragma once

bool R_CullBox(vec3_t mins, vec3_t maxs);
void R_RotateForEntity(entity_t* e);

void GL_DrawAliasFrame(aliashdr_t* paliashdr, int posenum);

void GL_DrawAliasShadow(aliashdr_t* paliashdr, int posenum);




void R_SetupAliasFrame(int frame, aliashdr_t* paliashdr);




void R_DrawAliasModel(entity_t* e);


void R_DrawEntitiesOnList(void);


void R_DrawViewModel(void);



void R_PolyBlend(void);


int SignbitsForPlane(mplane_t* out);


void R_SetFrustum(void);




void R_SetupFrame(void);


void MYgluPerspective(GLdouble fovy, GLdouble aspect,
	GLdouble zNear, GLdouble zFar);



void R_SetupGL(void);


void R_RenderScene(void);

void R_Clear(void);


void R_Mirror(void);

void R_RenderView(void);