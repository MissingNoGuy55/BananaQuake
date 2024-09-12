/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schmiedeberg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//
// spritegn.h: header file for sprite generation program
//

// **********************************************************
// * This file must be identical in the spritegen directory *
// * and in the Quake directory, because it's used to       *
// * pass data from one to the other via .spr files.        *
// **********************************************************

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "dictlib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#define SPRITE_VERSION	1
#define SPRITE_VERSION_GOLDSRC 2

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum {ST_SYNC=0, ST_RAND } synctype_t;
#endif

// TODO: shorten these?
typedef struct {
	int			ident;
	int			version;
	int			type;
	float		boundingradius;
	int			width;
	int			height;
	int			numframes;
	float		beamlength;
	synctype_t	synctype;
} dsprite_t;

typedef struct {
	char		ident[4];
	int			version;
	int			type;
	int			boundingradius;
	unsigned short		palptr;
	int			width;
	int			height;
	int			numframes;
	float		beamlength;
	synctype_t	synctype;
} dsprite_t_goldsrc;

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4

typedef struct {
	int			origin[2];
	int			width;
	int			height;
} dspriteframe_t;

typedef struct {
	int			origin[2];
	int			width;
	int			height;
} dspriteframe_goldsrc_t;

typedef struct {
	int			numframes;
} dspritegroup_t;

typedef struct {
	int			numframes;
} dspritegroup_goldsrc_t;

typedef struct {
	float	interval;
} dspriteinterval_t;

typedef struct {
	float	interval;
} dspriteinterval_goldsrc_t;

typedef enum { SPR_SINGLE=0, SPR_GROUP } spriteframetype_t;

typedef struct {
	spriteframetype_t	type;
} dspriteframetype_t;

typedef struct {
	spriteframetype_t	type;
} dspriteframetype_goldsrc_t;

#define IDSPRITEHEADER	(('P'<<24)+('S'<<16)+('D'<<8)+'I')
														// little-endian "IDSP"

