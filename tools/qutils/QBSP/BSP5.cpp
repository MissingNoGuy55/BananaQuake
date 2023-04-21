#include "BSP5.H"

CFace::CFace() : planenum(0), planeside(0), texturenum(0), outputnumber(0), numpoints(0)
{
	next = (CFace*)calloc(1, sizeof(CFace));
	original = (CFace*)calloc(1, sizeof(CFace));
	memset(contents, 0, sizeof(contents));
	memset(pts, 0, sizeof(pts));
	memset(edges, 0, sizeof(edges));
}

CFace::~CFace()
{
}

CFace::CFace(const CFace& face)
{

	memset(contents, 0, sizeof(contents));
	memset(pts, 0, sizeof(pts));
	memset(edges, 0, sizeof(edges));

	planenum = face.planenum;
	planeside = face.planeside;
	texturenum = face.texturenum;
	//original = face.original);
	outputnumber = face.outputnumber;
	numpoints = face.numpoints;

	for (int i = 0; i < 2; i++)
		contents[i] = face.contents[i];

	for (int i = 0; i < MAXEDGES; i++)
	{
		pts[i][0] = face.pts[i][0];
		pts[i][1] = face.pts[i][1];
		pts[i][2] = face.pts[i][2];
	}

	for (int i = 0; i < 2; i++)
		edges[i] = face.edges[i];

}

CFace::CFace(CFace* face)
{

	memset(contents, 0, sizeof(contents));
	memset(pts, 0, sizeof(pts));
	memset(edges, 0, sizeof(edges));

	planenum = face->planenum;
	planeside = face->planeside;
	texturenum = face->texturenum;
	original = face->original;
	outputnumber = face->outputnumber;
	numpoints = face->numpoints;

	for (int i = 0; i < 2; i++)
		contents[i] = face->contents[i];

	for (int i = 0; i < MAXEDGES; i++)
	{
		pts[i][0] = face->pts[i][0];
		pts[i][1] = face->pts[i][1];
		pts[i][2] = face->pts[i][2];
	}

	for (int i = 0; i < 2; i++)
		edges[i] = face->edges[i];

}
