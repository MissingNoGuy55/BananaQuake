#include "COMMON/cmdlib.h"
#include "COMMON/scriplib.h"
#include "COMMON/lbmlib.h"
#include "COMMON/wadlib.h"


extern  byte    *byteimage, *lbmpalette;
extern  int     byteimagewidth, byteimageheight;

#define SCRN(x,y)       (*(byteimage+(y)*byteimagewidth+x))

extern  byte    *lump_p;
extern  byte	*lumpbuffer;

extern	char	lumpname[];

