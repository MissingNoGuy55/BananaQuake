
#include "COMMON/CMDLIB.H"
#include "COMMON/MATHLIB.H"
#include "COMMON/BSPFILE.H"
#include "ENTITIES.H"
#include "THREADS.H"

#define	ON_EPSILON	0.1

#define	MAXLIGHTS			1024

void LoadNodes (char *file);
bool TestLine (vec3_t start, vec3_t stop);

void LightFace (int surfnum);
void LightLeaf (dleaf_t *leaf);

void MakeTnodes (dmodel_t *bm);

extern	float		scaledist;
extern	float		scalecos;
extern	float		rangescale;

extern	int		c_culldistplane, c_proper;

byte *GetFileSpace (int size);
extern	byte		*filebase;

extern	vec3_t	bsp_origin;
extern	vec3_t	bsp_xvector;
extern	vec3_t	bsp_yvector;

void TransformSample (vec3_t in, vec3_t out);
void RotateSample (vec3_t in, vec3_t out);

extern	bool	extrasamples;

extern	float		minlights[MAX_MAP_FACES];
