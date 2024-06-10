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
// quakedef.h -- primary header for client

//#define	GLTEST			// experimental stuff

#pragma once

#define TRACY_ENABLE

#define	QUAKE_GAME			// as opposed to utilities

#define	VERSION				1.09
#define	GLQUAKE_VERSION		1.00
#define	BANANAQUAKE_VERSION	0.10
#define	D3DQUAKE_VERSION	0.01
#define	WINQUAKE_VERSION	0.996
#define	LINUX_VERSION		1.30
#define	X11_VERSION			1.10

//define	PARANOID			// speed sapping error checking

typedef unsigned char byte;

#if (_WIN64) || (__x86_64__)
#define VOID_P long long
#else
#define VOID_P long
#endif

#ifdef QUAKE2
#define	GAMENAME	"id1"		// directory to look in by default
#else
#define	GAMENAME	"id1"
#endif

#include <cmath>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <setjmp.h>
#include <string>
#include <vector>
// #include <dsound.h>
#if (_WIN32) || (WIN64)
#include <windows.h>
#endif

using cxxstring = std::string;

template<typename T>
using cxxvector = std::vector<T>;

#include <SDL.h>

#ifdef GLQUAKE
#include <SDL_opengl.h>
#endif


#ifndef APIENTRY
#define	APIENTRY
#endif

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

void	VID_LockBuffer (void);
void	VID_UnlockBuffer (void);

#else

#define	VID_LockBuffer()
#define	VID_UnlockBuffer()

#endif

#if defined __i386__ // && !defined __sun__
#define id386	1
#else
#define id386	0
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY			0x550000
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_MSGLEN		64000		// max length of a reliable message
#define	MAX_DATAGRAM	64000 // 1024		// max length of unreliable message

//
// per-level limits
//
#define	MAX_EDICTS		32000			// FIXME: ouch! ouch! ouch!
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		2048			// these are sent over the net as bytes
#define	MAX_SOUNDS		2048			// so they cannot be blindly increased

// Missi: MAX_MODELS and MAX_SOUNDS were 256

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster

// stock defines

#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8
#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define	IT_INVISIBILITY			524288
#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

//===========================================
//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1<<HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1<<HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1<<HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1<<(23+2))
#define HIT_EMPATHY_SHIELDS (1<<(23+3))

//===========================================

typedef uintptr_t src_offset_t;

#define	MAX_SCOREBOARD		16
#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "mathlib.h"

#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"

#include "utils.h"

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		modelindex;
	int		frame;
	int		colormap;
	int		skin;
	int		effects;
} entity_state_t;

#include "wad.h"
#include "cvar.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "client.h"
#include "progs.h"

#include "server.h"
#include "strl_fn.h"	// Missi (1/10/2023)
#include "cfgfile.h"	// Missi (1/10/2023)
#include "draw.h"		// Missi: moved from below draw.h due to CSoftwareRenderer being defined there (4/24/2023)

#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
#include "d_iface.h"
#endif

#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"
#include "bgmusic.h"
#include "host.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use


//=============================================================================

extern bool noclip_anglehack;

//
// chase
//
extern	cvar_t	chase_active;

double Sys_DoubleTime();

void Chase_Init (void);
void Chase_Reset (void);
void Chase_Update (void);

//=============================================================================
// Missi: this is gross, but it's better than doing #ifdef to everything that these rely on... (4/24/2023)
//=============================================================================

#ifdef GLQUAKE
template<typename T = CGLRenderer>
#else
template<typename T = CSoftwareRenderer>
#endif
extern T* ResolveRenderer();

template<typename T>
T* ResolveRenderer()
{
#ifdef GLQUAKE
	return g_GLRenderer;
#else
	return g_SoftwareRenderer;
#endif
};
