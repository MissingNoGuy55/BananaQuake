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
// quakedef.h -- primary header for client

//#define	GLTEST			// experimental stuff

#pragma once

#define TRACY_ENABLE

#define	QUAKE_GAME			// as opposed to utilities

constexpr double VERSION				= 1.09;
constexpr double GLQUAKE_VERSION		= 1.00;
constexpr double BANANAQUAKE_VERSION	= 0.13;
constexpr double D3DQUAKE_VERSION		= 0.01;
constexpr double WINQUAKE_VERSION		= 0.996;
constexpr double LINUX_VERSION			= 1.30;
constexpr double X11_VERSION			= 1.10;

//define	PARANOID			// speed sapping error checking

constexpr int MINIMUM_MEMORY = 0x550000;
constexpr int MINIMUM_MEMORY_LEVELPAK = (MINIMUM_MEMORY + 0x100000);

constexpr short MAX_FILE_HANDLES = 64;

typedef unsigned char byte;

#ifdef __linux__
#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/Xxf86dga.h>    // Missi (4/30/2023)
#include <X11/extensions/xf86vmode.h>
#endif

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
#include <cstdint>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <setjmp.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <future>
#include <functional>
#include <filesystem>
// #include <dsound.h>
#if (_WIN32) || (WIN64)
#include <windows.h>
#endif

using cxxstring = std::string;
using cxxifstream = std::ifstream;
using cxxofstream = std::ofstream;
namespace fs = std::filesystem;
using cxxpath = fs::path;

#include <SDL.h>

#ifdef GLQUAKE
#include <SDL_opengl.h>
#include <SDL_image.h>
#endif


#ifndef APIENTRY
#define	APIENTRY
#endif

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

void	VID_LockBuffer ();
void	VID_UnlockBuffer ();

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
constexpr int CACHE_SIZE = 32;		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

constexpr int MAX_NUM_ARGVS = 50;

/*
// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2
*/

enum EOrientation
{
	PITCH,
	YAW,
	ROLL
};

constexpr int MAX_QPATH		=	64;			// max length of a quake game pathname
constexpr int MAX_OSPATH	=	128;	// max length of a filesystem pathname

constexpr double ON_EPSILON	=	0.1;	// point on plane side epsilon

constexpr int MAX_MSGLEN	=	64000;	// max length of a reliable message
constexpr int MAX_DATAGRAM	=	64000;	// 1024		// max length of unreliable message

//
// per-level limits
//
constexpr int	MAX_EDICTS	=	32000;			// Missi: was 600 (6/12/2024)
constexpr int	MAX_LIGHTSTYLES = 64;
constexpr int	MAX_MODELS	=	2048;			// these are sent over the net as bytes
constexpr int	MAX_SOUNDS	=	2048;			// so they cannot be blindly increased

// Missi: MAX_MODELS and MAX_SOUNDS were 256

constexpr int	SAVEGAME_COMMENT_LENGTH = 39;

constexpr int	MAX_STYLESTRING = 64;

//
// stats are integers communicated to the client by the server
//
constexpr int MAX_CL_STATS				= 32;
constexpr int STAT_HEALTH				= 0;
constexpr int STAT_FRAGS				= 1;
constexpr int STAT_WEAPON				= 2;
constexpr int STAT_AMMO					= 3;
constexpr int STAT_ARMOR				= 4;
constexpr int STAT_WEAPONFRAME			= 5;
constexpr int STAT_SHELLS				= 6;
constexpr int STAT_NAILS				= 7;
constexpr int STAT_ROCKETS				= 8;
constexpr int STAT_CELLS				= 9;
constexpr int STAT_ACTIVEWEAPON			= 10;
constexpr int STAT_TOTALSECRETS			= 11;
constexpr int STAT_TOTALMONSTERS		= 12;
constexpr int STAT_SECRETS				= 13;		// bumped on client side by svc_foundsecret
constexpr int STAT_MONSTERS				= 14;		// bumped by svc_killedmonster

// stock defines

typedef unsigned long long itemdef_t;

constexpr uint64_t IT_SHOTGUN				= 0x00000001;
constexpr uint64_t IT_SUPER_SHOTGUN		= 0x00000002;
constexpr uint64_t IT_NAILGUN				= 0x00000004;
constexpr uint64_t IT_SUPER_NAILGUN		= 0x00000008;
constexpr uint64_t IT_GRENADE_LAUNCHER	= 0x00000010;
constexpr uint64_t IT_ROCKET_LAUNCHER		= 0x00000020;
constexpr uint64_t IT_LIGHTNING			= 0x00000040;
constexpr uint64_t IT_SUPER_LIGHTNING		= 0x00000080;
constexpr uint64_t IT_SHELLS				= 0x00000100;
constexpr uint64_t IT_NAILS				= 0x00000200;
constexpr uint64_t IT_ROCKETS				= 0x00000400;
constexpr uint64_t IT_CELLS				= 0x00000800;
constexpr uint64_t IT_AXE					= 0x00001000;
constexpr uint64_t IT_ARMOR1				= 0x00002000;
constexpr uint64_t IT_ARMOR2				= 0x00004000;
constexpr uint64_t IT_ARMOR3				= 0x00008000;
constexpr uint64_t IT_SUPERHEALTH			= 0x00010000;
constexpr uint64_t IT_KEY1				= 0x00020000;
constexpr uint64_t IT_KEY2				= 0x00040000;
constexpr uint64_t IT_INVISIBILITY		= 0x00080000;
constexpr uint64_t IT_INVULNERABILITY		= 0x00100000;
constexpr uint64_t IT_SUIT				= 0x00200000;
constexpr uint64_t IT_QUAD				= 0x00400000;
constexpr uint64_t IT_SIGIL1				= 0x10000000;
constexpr uint64_t IT_SIGIL2				= 0x20000000;
constexpr uint64_t IT_SIGIL3				= 0x40000000;
constexpr uint64_t IT_SIGIL4				= 0x80000000;

constexpr uint64_t IT_BULLET_TIME			= 0x100000000;

//===========================================
//rogue changed and added defines

constexpr uint64_t RIT_SHELLS				= 0x00000080;
constexpr uint64_t RIT_NAILS				= 0x00000100;
constexpr uint64_t RIT_ROCKETS			= 0x00000200;
constexpr uint64_t RIT_CELLS				= 0x00000400;
constexpr uint64_t RIT_AXE				= 0x00000800;
constexpr uint64_t RIT_LAVA_NAILGUN		= 0x00001000;
constexpr uint64_t RIT_LAVA_SUPER_NAILGUN	= 0x00002000;
constexpr uint64_t RIT_MULTI_GRENADE		= 0x00004000;
constexpr uint64_t RIT_MULTI_ROCKET		= 0x00008000;
constexpr uint64_t RIT_PLASMA_GUN			= 0x00010000;
constexpr uint64_t RIT_ARMOR1				= 0x00800000;
constexpr uint64_t RIT_ARMOR2				= 0x01000000;
constexpr uint64_t RIT_ARMOR3				= 0x02000000;
constexpr uint64_t RIT_LAVA_NAILS			= 0x04000000;
constexpr uint64_t RIT_PLASMA_AMMO		= 0x08000000;
constexpr uint64_t RIT_MULTI_ROCKETS		= 0x10000000;
constexpr uint64_t RIT_SHIELD				= 0x20000000;
constexpr uint64_t RIT_ANTIGRAV			= 0x40000000;
constexpr uint64_t RIT_SUPERHEALTH		= 0x80000000;

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
constexpr uint64_t HIT_PROXIMITY_GUN_BIT	= 16;
constexpr uint64_t HIT_MJOLNIR_BIT			= 7;
constexpr uint64_t HIT_LASER_CANNON_BIT	= 23;
constexpr uint64_t HIT_PROXIMITY_GUN		= (1<<HIT_PROXIMITY_GUN_BIT);
constexpr uint64_t HIT_MJOLNIR				= (1<<HIT_MJOLNIR_BIT);
constexpr uint64_t HIT_LASER_CANNON		= (1<<HIT_LASER_CANNON_BIT);
constexpr uint64_t HIT_WETSUIT				= (1<<(23+2));
constexpr uint64_t HIT_EMPATHY_SHIELDS		= (1<<(23+3));

//===========================================

typedef uintptr_t src_offset_t;

constexpr int MAX_SCOREBOARD		= 16;
constexpr int MAX_SCOREBOARDNAME	= 32;

constexpr int SOUND_CHANNELS		= 8;

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "unzip.h"
#include "mathlib.h"

#include "common.h"
#include "vpkfile.h"
#include "bspfile.h"
#include "bspfile_goldsrc.h"
#include "bspfile_source.h"
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
#include "gl_images.h"
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

#ifdef DEBUG
namespace QuakeDebugOverlay
{
	void DrawDebugOverlay(void* parm1, void* parm2, void* parm3);
	void DrawDebugBox(GLfloat* vColor, Vector3* mins, Vector3* maxs, double fTime);

	extern CQTimer m_DebugOverlayTimer;
};
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

void Chase_Init ();
void Chase_Reset ();
void Chase_Update ();

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
