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
// zone.cpp

// Missi: most functions moved to zone.h due to turning most of the memory into template classes (5/2/2023)

#include "quakedef.h"

cvar_t	zone_debug = { "zone_debug", "0" };

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

CMemCache* g_MemCache;
CMemZone* mainzone = new CMemZone;
CMemCacheSystem		cache_head = {};

/*
template<typename T>
extern CMemCacheSystem* test_head;

template<typename T>
CMemCacheSystem* test_head = new CMemCacheSystem;

template<typename T>
CMemCacheSystem		cache_head = *test_head<T>;
*/

byte*		hunk_base;
size_t		hunk_size;

size_t		hunk_low_used;
size_t		hunk_high_used;

bool		hunk_tempactive;
size_t		hunk_tempmark;


CMemZone::~CMemZone()
{
}

char* CMemZone::Z_Strdup(const char* s)
{
	size_t sz = strlen(s) + 1;
	char* ptr = (char*)mainzone->Z_Malloc<char>(sz);
	memcpy(ptr, s, sz);
	return ptr;
}

//============================================================================

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/

char* CMemCache::Hunk_Strdup(const char* s, const char* name)
{
	size_t sz = strlen(s) + 1;
	char* ptr = (char*)Hunk_AllocName<char>(sz, name);
	memcpy(ptr, s, sz);
	return ptr;
}

void CMemCache::Hunk_Check ()
{
	hunk_t	*h;
	
	for (h = (hunk_t *)hunk_base ; (byte *)h != hunk_base + hunk_low_used ; )
	{
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trahsed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");
		h = (hunk_t *)((byte *)h+h->size);
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void CMemCache::Hunk_Print (bool all)
{
	hunk_t	*h, *next, *endlow, *starthigh, *endhigh;
	int		count, sum;
	int		totalblocks;
	char	name[9];

	name[8] = 0;
	count = 0;
	sum = 0;
	totalblocks = 0;
	
	h = (hunk_t *)hunk_base;
	endlow = (hunk_t *)(hunk_base + hunk_low_used);
	starthigh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	endhigh = (hunk_t *)(hunk_base + hunk_size);

	Con_Printf ("          :%8i total hunk size\n", hunk_size);
	Con_Printf ("-------------------------\n");

	while (1)
	{
	//
	// skip to the high hunk if done with low hunk
	//
		if ( h == endlow )
		{
			Con_Printf ("-------------------------\n");
			Con_Printf ("          :%8i REMAINING\n", hunk_size - hunk_low_used - hunk_high_used);
			Con_Printf ("-------------------------\n");
			h = starthigh;
		}
		
	//
	// if totally done, break
	//
		if ( h == endhigh )
			break;

	//
	// run consistancy checks
	//
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trahsed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");
			
		next = (hunk_t *)((byte *)h+h->size);
		count++;
		totalblocks++;
		sum += h->size;

	//
	// print the single block
	//
		memcpy (name, h->name, 8);
		if (all)
			Con_Printf ("%8p :%8i %8s\n",h, h->size, name);
			
	//
	// print the total
	//
		if (next == endlow || next == endhigh || 
		strncmp (h->name, next->name, 8) )
		{
			if (!all)
				Con_Printf ("          :%8i %8s (TOTAL)\n",sum, name);
			count = 0;
			sum = 0;
		}

		h = next;
	}

	Con_Printf ("-------------------------\n");
	Con_Printf ("%8i total blocks\n", totalblocks);
	
}

int	CMemCache::Hunk_LowMark ()
{
	return hunk_low_used;
}

void CMemCache::Hunk_FreeToLowMark (int mark)
{
	if (mark < 0 || mark > hunk_low_used)
		Sys_Error ("Hunk_FreeToLowMark: bad mark %i", mark);
	memset (hunk_base + mark, 0, hunk_low_used - mark);
	hunk_low_used = mark;
}

int	CMemCache::Hunk_HighMark ()
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}

	return hunk_high_used;
}

void CMemCache::Hunk_FreeToHighMark (int mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
		Sys_Error ("Hunk_FreeToHighMark: bad mark %i", mark);
	memset (hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
	hunk_high_used = mark;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

/*
============
Cache_Report

============
*/
void CMemCache::Cache_Report ()
{
	Con_DPrintf ("%4.1f megabyte data cache\n", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Compact

============
*/
void CMemCache::Cache_Compact ()
{
}

CMemZone::CMemZone() : size(0), rover(nullptr)
{
}

CMemCache::CMemCache()
{
}
/*
CMemCacheSystem::CMemCacheSystem()
{
	size = 0;

	//next = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//prev = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//lru_next = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//lru_prev = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));

	//next = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//prev = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//lru_next = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));
	//lru_prev = (class CMemCacheSystem*)calloc(1, sizeof(class CMemCacheSystem));

	ConstructList();

}

void CMemCacheSystem::ConstructList()
{
	next = NULL;
	prev = NULL;
	lru_next = NULL;
	lru_prev = NULL;
}

*/

void CMemCache::Cache_UnlinkLRU(CMemCacheSystem* cs)
{
	if (cs)
	{
		if (!cs->lru_next || !cs->lru_prev)
			Sys_Error("Cache_UnlinkLRU: NULL link");

		if (cs->lru_next && cs->lru_prev)
		{
			cs->lru_next->lru_prev = cs->lru_prev;
			cs->lru_prev->lru_next = cs->lru_next;
		}

		cs->lru_prev = cs->lru_next = NULL;
	}
}

void CMemCache::Cache_MakeLRU(CMemCacheSystem* cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

//============================================================================
