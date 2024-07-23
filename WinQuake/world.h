/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// world.h

#pragma once

typedef struct
{
	vec3_t	normal;
	float	dist;
} plane_t;

typedef struct
{
	bool	allsolid;	// if true, plane is not valid
	bool	startsolid;	// if true, the initial point was in a solid area
	bool	inopen, inwater;
	float	fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t	endpos;			// final position
	plane_t	plane;			// surface normal at impact
	edict_t* ent;			// entity the surface is on
} trace_t;

void TraceLine(vec3_t start, vec3_t end, vec3_t impact, trace_t trace);

#define	MOVE_NORMAL		0
#define	MOVE_NOMONSTERS	1
#define	MOVE_MISSILE	2
#define MOVE_BRUSH		3
