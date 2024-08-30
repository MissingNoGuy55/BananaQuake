#pragma once

#define BSPVERSION_SOURCE_TROIKA	17
#define BSPVERSION_SOURCE_2003		18
#define BSPVERSION_SOURCE_2004		19
#define BSPVERSION_SOURCE_2007		20
#define BSPVERSION_SOURCE_2009		21
#define BSPVERSION_SOURCE_STRATA	25
#define BSPVERSION_SOURCE_CONTAGION	27

#define	HEADER_LUMPS_SOURCE			64

enum e_source2004_lumps : unsigned char
{
	SOURCE_LUMP_ENTITIES = 0,
	SOURCE_LUMP_PLANES,
	SOURCE_LUMP_TEXDATA,
	SOURCE_LUMP_VERTEXES,
	SOURCE_LUMP_VISIBILITY,
	SOURCE_LUMP_NODES,
	SOURCE_LUMP_TEXINFO,
	SOURCE_LUMP_FACES,
	SOURCE_LUMP_LIGHTING,
	SOURCE_LUMP_OCCLUSION,
	SOURCE_LUMP_LEAFS,
	SOURCE_LUMP_UNUSED,			// Missi: LUMP_FACEIDS in Orange Box (8/3/2024)
	SOURCE_LUMP_EDGES,
	SOURCE_LUMP_SURFEDGES,
	SOURCE_LUMP_MODELS,
	SOURCE_LUMP_WORLDLIGHTS,
	SOURCE_LUMP_LEAFFACES,
	SOURCE_LUMP_LEAFBRUSHES,
	SOURCE_LUMP_BRUSHES,
	SOURCE_LUMP_BRUSHSIDES,
	SOURCE_LUMP_AREAS,
	SOURCE_LUMP_AREAPORTALS,
	SOURCE_LUMP_PORTALS,		// Missi: LUMP_UNUSED0 in Orange Box and LUMP_PROPCOLLISION in L4D2 (8/3/2024)
	SOURCE_LUMP_CLUSTERS,		// Missi: LUMP_UNUSED1 in Orange Box and LUMP_PROPHULLS in L4D2 (8/3/2024)
	SOURCE_LUMP_PORTALVERTS,	// Missi: LUMP_UNUSED2 in Orange Box and LUMP_PROPHULLVERTS in L4D2 (8/3/2024)
	SOURCE_LUMP_CLUSTERPORTALS,	// Missi: LUMP_UNUSED3 in Orange Box and LUMP_PROPTRIS in L4D2 (8/3/2024)
	SOURCE_LUMP_DISPINFO,
	SOURCE_LUMP_ORIGINALFACES,
	SOURCE_LUMP_UNUSED1,		// Missi: LUMP_PHYSDISP in Orange Box (8/3/2024)
	SOURCE_LUMP_PHYSCOLLIDE,
	SOURCE_LUMP_VERTNORMALS,
	SOURCE_LUMP_VERTNORMALINDICES,
	SOURCE_LUMP_DISP_LIGHTMAP_ALPHAS,
	SOURCE_LUMP_DISP_VERTS,
	SOURCE_LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS,
	SOURCE_LUMP_GAME_LUMP,
	SOURCE_LUMP_LEAFWATERDATA,
	SOURCE_LUMP_PRIMITIVES,
	SOURCE_LUMP_PRIMVERTS,
	SOURCE_LUMP_PRIMINDICES,
	SOURCE_LUMP_PAKFILE,
	SOURCE_LUMP_CLIPPORTALVERTS,
	SOURCE_LUMP_CUBEMAPS,
	SOURCE_LUMP_TEXDATA_STRING_DATA,
	SOURCE_LUMP_TEXDATA_STRING_TABLE,
	SOURCE_LUMP_OVERLAYS,
	SOURCE_LUMP_LEAFMINDISTTOWATER,
	SOURCE_LUMP_FACE_MACRO_TEXTURE_INFO,
	SOURCE_LUMP_DISP_TRIS,
	SOURCE_LUMP_PHYSCOLLIDESURFACE		// Missi: LUMP_PROP_BLOB in L4D2 (8/3/2024)

	// Missi: everything after is Source 2006 and onwards

};

enum e_source2006_lumps : unsigned char
{
	LUMP_WATEROVERLAYS = 50,
	LUMP_LIGHTMAPPAGES,
	LUMP_LIGHTMAPPAGEINFOS,
	LUMP_LIGHTING_HDR,
	LUMP_WORLDLIGHTS_HDR,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR,
	LUMP_LEAF_AMBIENT_LIGHTING,
	LUMP_XZIPPAKFILE,
	LUMP_FACES_HDR,
	LUMP_MAP_FLAGS
};

enum e_source2007_lumps : unsigned char
{
	SOURCE_LUMP_FACEIDS = 11,
	SOURCE_LUMP_UNUSED0_2007 = 22,
	SOURCE_LUMP_UNUSED1_2007 = 23,
	SOURCE_LUMP_UNUSED2_2007 = 24,
	SOURCE_LUMP_UNUSED3_2007 = 25,
	SOURCE_LUMP_PHYSDISP = 28,
	SOURCE_LUMP_LEAF_AMBIENT_INDEX_HDR = 51,
	SOURCE_LUMP_LEAF_AMBIENT_INDEX = 52,
	SOURCE_LUMP_OVERLAY_FADES = 60
};

struct lump_source2004_t
{
	int    fileofs;      // offset into file (bytes)
	int    filelen;      // length of lump (bytes)
	int    version;      // lump format version
	char   fourCC[4];    // lump ident code
};

struct dheader_source2004_t
{
	int     ident;                  // BSP file identifier
	int     version;                // BSP file version
	lump_source2004_t  lumps[HEADER_LUMPS_SOURCE];    // lump directory array
	int     mapRevision;            // the map's revision (iteration, version) number
};

struct dplane_source2004_t
{
	vec3_t  normal;   // normal vector
	float   dist;     // distance from origin
	int     type;     // plane axis identifier
};

struct dedge_source2004_t
{
	unsigned short  v[2];  // vertex indices
};

struct dface_source2004_t
{
	unsigned short  planenum;               // the plane number
	byte            side;                   // faces opposite to the node's plane direction
	byte            onNode;                 // 1 of on node, 0 if in leaf
	int             firstedge;              // index into surfedges
	short           numedges;               // number of surfedges
	short           texinfo;                // texture info
	short           dispinfo;               // displacement info
	short           surfaceFogVolumeID;     // ?
	byte            styles[4];              // switchable lighting info
	int             lightofs;               // offset into lightmap lump
	float           area;                   // face area in units^2
	int             LightmapTextureMinsInLuxels[2]; // texture lighting info
	int             LightmapTextureSizeInLuxels[2]; // texture lighting info
	int             origFace;               // original face this was split from
	unsigned short  numPrims;               // primitives
	unsigned short  firstPrimID;
	unsigned int    smoothingGroups;        // lightmap smoothing group
};

struct dbrush_source2004_t
{
	int    firstside;     // first brushside
	int    numsides;      // number of brushsides
	int    contents;      // contents flags
};

struct dbrushside_source2004_t
{
	unsigned short  planenum;     // facing out of the leaf
	short           texinfo;      // texture info
	short           dispinfo;     // displacement info
	short           bevel;        // is the side a bevel plane?
};

struct dnode_source2004_t
{
	int             planenum;       // index into plane array
	int             children[2];    // negative numbers are -(leafs + 1), not nodes
	short           mins[3];        // for frustum culling
	short           maxs[3];
	unsigned short  firstface;      // index into face array
	unsigned short  numfaces;       // counting both sides
	short           area;           // If all leaves below this node are in the same area, then
	// this is the area index. If not, this is -1.
	short           paddding;       // pad to 32 bytes length
};

struct dleaf_source2004_t
{
	int             contents;             // OR of all brushes (not needed?)
	short           cluster;              // cluster this leaf is in
	short           area : 9;               // area this leaf is in
	short           flags : 7;              // flags
	short           mins[3];              // for frustum culling
	short           maxs[3];
	unsigned short  firstleafface;        // index into leaffaces
	unsigned short  numleaffaces;
	unsigned short  firstleafbrush;       // index into leafbrushes
	unsigned short  numleafbrushes;
	short           leafWaterDataID;      // -1 for not in water

	//!!! NOTE: for lump version 0 (usually in maps of version 19 or lower) uncomment the next line
	//CompressedLightCube   ambientLighting;      // Precaculated light info for entities.
	short                 padding;              // padding to 4-byte boundary
};

struct texinfo_source2004_t
{
	float   textureVecs[2][4];    // [s/t][xyz offset]
	float   lightmapVecs[2][4];   // [s/t][xyz offset] - length is in units of texels/area
	int     flags;                // miptex flags overrides
	int     texdata;              // Pointer to texture name, size, etc.
};

struct dtexdata_source2004_t
{
	Vector3  reflectivity;            // RGB reflectivity
	int     nameStringTableID;       // index into TexdataStringTable
	int     width, height;           // source image
	int     view_width, view_height;
};

struct dmodel_source2004_t
{
	Vector3  mins, maxs;            // bounding box
	Vector3  origin;                // for sounds or lights
	int     headnode;              // index into node array
	int     firstface, numfaces;   // index into face array
};

struct dvis_source2004_t
{
	int	numclusters;
	int	byteofs[MAX_MAP_VISIBILITY][2];
};

struct dgamelump_t
{
	int             id;        // gamelump ID
	unsigned short  flags;     // flags
	unsigned short  version;   // gamelump version
	int             fileofs;   // offset to this gamelump
	int             filelen;   // length
};

struct dgamelumpheader_t
{
	int lumpCount;  // number of game lumps
	dgamelump_t gamelump[32];
};

struct StaticPropDictLump_t
{
	int		dictEntries;
	char	name[128][64];	// model name
};

struct StaticPropLeafLump_t
{
	int leafEntries;
	unsigned short	leaf[128];
};

typedef struct _color32_s
{
	int r;
	int g;
	int b;
} color32_t;

struct StaticPropLump_t
{
	// v4
	Vector3          Origin;            // origin
	Vector3          Angles;            // orientation (pitch yaw roll)

	// v4
	unsigned short  PropType;          // index into model name dictionary
	unsigned short  FirstLeaf;         // index into leaf array
	unsigned short  LeafCount;
	unsigned char   Solid;             // solidity type
	// every version except v7*
	unsigned char   Flags;
	// v4 still
	int             Skin;              // model skin numbers
	float           FadeMinDist;
	float           FadeMaxDist;
	Vector3          LightingOrigin;    // for lighting
	// since v5
	float           ForcedFadeScale;   // fade distance scale
	// v6, v7, and v7* only
	unsigned short  MinDXLevel;        // minimum DirectX version to be visible
	unsigned short  MaxDXLevel;        // maximum DirectX version to be visible
	// v7* only
	unsigned int    iFlags;
	unsigned short  LightmapResX;      // lightmap image width
	unsigned short	LightmapResY;      // lightmap image height
	// since v8
	unsigned char   MinCPULevel;
	unsigned char   MaxCPULevel;
	unsigned char   MinGPULevel;
	unsigned char   MaxGPULevel;
	// since v7
	color32_t       DiffuseModulation; // per instance color and alpha modulation
	// v9 and v10 only
	bool            DisableX360;       // if true, don't show on XBox 360 (4-bytes long)
	// since v10
	unsigned int    FlagsEx;           // Further bitflags.
	// since v11
	float           UniformScale;      // Prop scale
};

// These can be used to index g_ChildNodeIndexMul.
enum
{
	CHILDNODE_UPPER_RIGHT = 0,
	CHILDNODE_UPPER_LEFT = 1,
	CHILDNODE_LOWER_LEFT = 2,
	CHILDNODE_LOWER_RIGHT = 3
};


// Corner indices. Used to index m_CornerNeighbors.
enum
{
	CORNER_LOWER_LEFT = 0,
	CORNER_UPPER_LEFT = 1,
	CORNER_UPPER_RIGHT = 2,
	CORNER_LOWER_RIGHT = 3
};

// These edge indices must match the edge indices of the CCoreDispSurface.
enum
{
	NEIGHBOREDGE_LEFT = 0,
	NEIGHBOREDGE_TOP = 1,
	NEIGHBOREDGE_RIGHT = 2,
	NEIGHBOREDGE_BOTTOM = 3
};

// These denote where one dispinfo fits on another.
// Note: tables are generated based on these indices so make sure to update
//       them if these indices are changed.
typedef enum
{
	CORNER_TO_CORNER = 0,
	CORNER_TO_MIDPOINT = 1,
	MIDPOINT_TO_CORNER = 2
} e_neighborspan;


// These define relative orientations of displacement neighbors.
typedef enum
{
	ORIENTATION_CCW_0 = 0,
	ORIENTATION_CCW_90 = 1,
	ORIENTATION_CCW_180 = 2,
	ORIENTATION_CCW_270 = 3
} e_neighbor_orientation;

// NOTE: see the section above titled "displacement neighbor rules".
struct dispsubneighbor_t
{
	unsigned short		m_iNeighbor;		// This indexes into ddispinfos.
	// 0xFFFF if there is no neighbor here.

	unsigned char		m_NeighborOrientation;		// (CCW) rotation of the neighbor wrt this displacement.

	// These use the NeighborSpan type.
	unsigned char		m_Span;						// Where the neighbor fits onto this side of our displacement.
	unsigned char		m_NeighborSpan;				// Where we fit onto our neighbor.
};

struct dispneighbor_t
{
	dispsubneighbor_t	m_SubNeighbors[2];
};


struct dispcornerneighbors_t
{
	unsigned short	m_Neighbors[4];	// indices of neighbors.
	unsigned char	m_nNeighbors;
};

struct ddispinfo_t
{
	Vector3					startPosition;                // start position used for orientation
	int						DispVertStart;                // Index into LUMP_DISP_VERTS.
	int						DispTriStart;                 // Index into LUMP_DISP_TRIS.
	int						power;                        // power - indicates size of surface (2^power 1)
	int						minTess;                      // minimum tesselation allowed
	float					smoothingAngle;               // lighting smoothing angle
	int						contents;                     // surface contents
	unsigned short			MapFace;                      // Which map face this displacement comes from.
	int						LightmapAlphaStart;           // Index into ddisplightmapalpha.
	int						LightmapSamplePositionStart;  // Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.
	dispneighbor_t			EdgeNeighbors[4];             // Indexed by NEIGHBOREDGE_ defines.
	dispcornerneighbors_t	CornerNeighbors[4];           // Indexed by CORNER_ defines.
	unsigned int			AllowedVerts[10];             // active verticies
};

struct dDispVert
{
	Vector3  vec;    // Vector field defining displacement volume.
	float   dist;   // Displacement distances.
	float   alpha;  // "per vertex" alpha values.
};

struct dDispTri
{
	unsigned short Tags;   // Displacement triangle tags.
};

struct dcubemapsample_t
{
	int    origin[3];    // position of light snapped to the nearest integer
	int    size;         // resolution of cubemap, 0 - default
};

struct doverlay_t
{
	int             Id;
	short           TexInfo;
	unsigned short  FaceCountAndRenderOrder;
	int             Ofaces[64];
	float           U[2];
	float           V[2];
	Vector3          UVPoints[4];
	Vector3          Origin;
	Vector3          BasisNormal;
};

struct ColorRGBExp32
{
	byte r, g, b;
	signed char exponent;
};

struct CompressedLightCube
{
	ColorRGBExp32 m_Color[6];
};

struct dleafambientlighting_t
{
	CompressedLightCube	cube;
	byte x;		// fixed point fraction of leaf bounds
	byte y;		// fixed point fraction of leaf bounds
	byte z;		// fixed point fraction of leaf bounds
	byte pad;	// unused
};

struct doccluderpolydata_t
{
	int	firstvertexindex;	// index into doccludervertindices
	int	vertexcount;		// amount of vertex indices
	int	planenum;
};

struct doccluderdata_t
{
	int	flags;
	int	firstpoly;	// index into doccluderpolys
	int	polycount;	// amount of polygons
	Vector3	mins;	        // minima of all vertices
	Vector3	maxs;	        // maxima of all vertices
	// since v1
	int	area;
};

struct doccluder_t
{
	int			count;
	doccluderdata_t		data[64];
	int			polyDataCount;
	doccluderpolydata_t	polyData[64];
	int			vertexIndexCount;
	int			vertexIndices[64];
};

constexpr unsigned int MAX_MAP_TEXDATA = 2048;
constexpr unsigned int MAX_MAP_TEXDATA_STRING_DATA = 256000;

// little-endian "VBSP"   0x50534256
#define VBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'V')