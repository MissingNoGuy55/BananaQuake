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
// Z_zone.c

#include "quakedef.h"

cvar_t	zone_debug = { "zone_debug", "0" };

void Cache_FreeLow (int new_low_hunk);
void Cache_FreeHigh (int new_high_hunk);


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

/*
========================
Z_ClearZone
========================
*/
void CMemZone::Z_ClearZone (CMemZone *zone, int size)
{
	CMemBlock<byte>	*block;
	
// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
		(CMemBlock<byte> *)( (byte *)zone + sizeof(CMemZone) );
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;
	
	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(CMemZone);
}


CMemZone::CMemZone() : size(0), blocklist(NULL), rover(NULL)
{
}

CMemZone::~CMemZone()
{
	memset(&blocklist, 0, sizeof(blocklist));
	rover = NULL;
}

/*
========================
Z_Free
========================
*/
void CMemZone::Z_Free (void *ptr)
{
	CMemBlock<byte>	*block, *other;
	
	if (!ptr)
		Sys_Error ("Z_Free: NULL pointer");

	block = (CMemBlock<byte> *) ( (byte *)ptr - sizeof(CMemBlock<byte>));
	if (block->id != ZONEID)
		Sys_Error ("Z_Free: freed a pointer without ZONEID");
	if (block->tag == 0)
		Sys_Error ("Z_Free: freed a freed pointer");

	block->tag = 0;		// mark as free
	
	other = block->prev;
	if (!other->tag)
	{	// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == rover)
			rover = other;
		block = other;
	}
	
	other = block->next;
	if (!other->tag)
	{	// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == rover)
			rover = block;
	}
}

void* CMemZone::Z_TagMalloc (int size, int tag)
{
	int		extra = 0;
	CMemBlock<byte> *start, *rover, *m_new, *base;

	// start = rover = m_new = base = new CMemBlock<byte>(size);

	if (!tag)
		Sys_Error ("Z_TagMalloc: tried to use a 0 tag");

//
// scan through the block list looking for the first free block
// of sufficient size
//
	size += sizeof(CMemBlock<byte>);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 7) & ~7;		// align to 8-byte boundary
	
	base = g_MemCache->mainzone->rover; // = mainzone->rover;
	start = base->prev;
	
	do
	{
		if (this->rover == start)	// scaned all the way around the list
			return NULL;
		if (this->rover->tag)
			base = this->rover = this->rover->next;
		else
			this->rover = this->rover->next;
	} while (base->tag || base->size < size);
	
//
// found a block big enough
//
	extra = base->size - size;
	if (extra >  MINFRAGMENT)
	{	// there will be a free fragment after the allocated block
		m_new = (CMemBlock<byte> *) ((byte *)base + size );
		m_new->size = extra;
		m_new->tag = 0;			// free block
		m_new->prev = base;
		m_new->id = ZONEID;
		m_new->next = base->next;
		m_new->next->prev = m_new;
		base->next = m_new;
		base->size = size;
	}
	
	base->tag = tag;				// no longer a free block
	
	rover = base->next;	// next allocation will start looking here
	
	base->id = ZONEID;

// marker for memory trash testing
	*(int *)((byte *)base + base->size - 4) = ZONEID;

	auto test = ((byte *)base + sizeof(CMemBlock<byte>));

	return (void*)((byte*)base + sizeof(CMemBlock<byte>));
}


/*
========================
Z_Print
========================
*/
void CMemZone::Z_Print (CMemZone *zone)
{
	CMemBlock<byte>	*block;
	
	Con_Printf ("zone size: %i  location: %p\n",size,this);
	
	for (block = zone->blocklist.next ; ; block = block->next)
	{
		Con_Printf ("block:%p    size:%7i    tag:%3i\n",
			block, block->size, block->tag);
		
		if (block->next == &zone->blocklist)
			break;			// all blocks have been hit	
		if ( (byte *)block + block->size != (byte *)block->next)
			Con_Printf ("ERROR: block size does not touch the next block\n");
		if ( block->next->prev != block)
			Con_Printf ("ERROR: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Con_Printf ("ERROR: two consecutive free blocks\n");
	}
}


/*
========================
Z_CheckHeap
========================
*/
void CMemZone::Z_CheckHeap(void)
{
	CMemBlock<byte>	*block;
	
	for (block = blocklist.next ; ; block = block->next)
	{
		if (block->next == &blocklist)
			break;			// all blocks have been hit	
		if ( (byte *)block + block->size != (byte *)block->next)
			Sys_Error ("Z_CheckHeap: block size does not touch the next block\n");
		if ( block->next->prev != block)
			Sys_Error ("Z_CheckHeap: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Sys_Error ("Z_CheckHeap: two consecutive free blocks\n");
	}
}

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[24];
} hunk_t;

byte	*hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

bool	hunk_tempactive;
int		hunk_tempmark;

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void CMemCache::Hunk_Check (void)
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

/*===================
Hunk_AllocName

Missi: Allocates a named block of memory on the hunk. Castable to any type. Use only for things like models and textures. If you need small, possibly volatile memory, use CMemZone::Z_TagMalloc instead.
===================*/
void* CMemCache::Hunk_AllocName (int size, const char *name)
{
	hunk_t	*h;
	
#ifdef PARANOID
	Hunk_Check ();
#endif

	if (size < 0)
		Sys_Error ("Hunk_Alloc: bad size: %i", size);
		
	size = sizeof(hunk_t) + ((size+15)&~15);
	
	if (hunk_size - hunk_low_used - hunk_high_used < size)
		Sys_Error ("Hunk_Alloc: failed on %i bytes",size);
	
	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow (hunk_low_used);

	memset (h, 0, size);
	
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy (h->name, name, 24);
	
	return (void *)(h+1);
}

/*===================
Hunk_Alloc

Missi: Allocates a block of memory on the hunk. Usually used for things like textures and models. Use CMemZone if you need string or volatile memory allocation.

This function exists just for brevity; it calls Hunk_AllocName with a const char parameter of "unknown" (3/8/2022)
===================*/
void* CMemCache::Hunk_Alloc (int size)
{
	return Hunk_AllocName (size, "unknown");
}

int	CMemCache::Hunk_LowMark (void)
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

int	CMemCache::Hunk_HighMark (void)
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
===================
Hunk_HighAllocName
===================
*/
void* CMemCache::Hunk_HighAllocName (int size, const char *name)
{
	hunk_t	*h;

	if (size < 0)
		Sys_Error ("Hunk_HighAllocName: bad size: %i", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

#ifdef PARANOID
	Hunk_Check ();
#endif

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		Con_Printf ("Hunk_HighAlloc: failed on %i bytes\n",size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh (hunk_high_used);

	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

	memset (h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	Q_strncpy (h->name, name, 8);

	return (void *)(h+1);
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void* CMemCache::Hunk_TempAlloc (int size)
{
	void	*buf = 0;

	size = (size+15)&~15;
	
	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}
	
	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName (size, "temp");

	hunk_tempactive = true;

	return buf;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

/*
===========
Cache_Move
===========
*/
void CMemCache::Cache_Move ( CMemCacheSystem *c)
{
	CMemCacheSystem		*c_new;

// we are clearing up space at the bottom, so only allocate it late
	c_new = Cache_TryAlloc (c->size, true);
	if (c_new)
	{
//		Con_Printf ("cache_move ok\n");

		Q_memcpy (c_new +1, c+1, c->size - sizeof(CMemCacheSystem) );
		c_new->user = c->user;
		Q_memcpy (c_new->name, c->name, sizeof(c_new->name));
		Cache_Free (c->user);
		c_new->user->data = (void *)(c_new +1);
	}
	else
	{
//		Con_Printf ("cache_move failed\n");

		Cache_Free (c->user);		// tough luck...
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void CMemCache::Cache_FreeLow (int new_low_hunk)
{
	CMemCacheSystem	*c;
	
	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ((byte *)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( c );	// reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void CMemCache::Cache_FreeHigh (int new_high_hunk)
{
	CMemCacheSystem	*c = NULL, *prev = NULL;
	
	while (1)
	{
		c = cache_head.prev;
		if (c)
		{
			if (c == &cache_head)
				return;		// nothing in cache at all
			if ((byte*)c + c->size <= hunk_base + hunk_size - new_high_hunk)
				return;		// there is space to grow the hunk
			if (c == prev)
				Cache_Free(c->user);	// didn't move out of the way
			else
			{
				Cache_Move(c);	// try to move it
				prev = c;
			}
		}
	}
}

void CMemCache::Cache_UnlinkLRU (CMemCacheSystem *cs)
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

void CMemCache::Cache_MakeLRU (CMemCacheSystem *cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
CMemCacheSystem* CMemCache::Cache_TryAlloc (int size, bool nobottom)
{
	CMemCacheSystem	*cs, *c_new;
	
// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			Sys_Error ("Cache_TryAlloc: %i is greater then free hunk", size);

		c_new = (CMemCacheSystem *) (hunk_base + hunk_low_used);
		memset (c_new, 0, sizeof(*c_new));
		c_new->size = size;

		cache_head.prev = cache_head.next = c_new;
		c_new->prev = c_new->next = &cache_head;
		
		Cache_MakeLRU (c_new);
		m_CacheSystem = c_new;
		return c_new;
	}
	
// search from the bottom up for space

	c_new = (CMemCacheSystem *) (hunk_base + hunk_low_used);
	cs = cache_head.next;
	
	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ( (byte *)cs - (byte *)c_new >= size)
			{	// found space
				memset (c_new, 0, sizeof(*c_new));
				c_new->size = size;
				
				c_new->next = cs;
				c_new->prev = cs->prev;
				cs->prev->next = c_new;
				cs->prev = c_new;
				
				Cache_MakeLRU (c_new);
	
				m_CacheSystem = c_new;
				return c_new;
			}
		}

	// continue looking		
		c_new = (CMemCacheSystem *)((byte *)cs + cs->size);
		cs = cs->next;

	} while (cs != &cache_head);
	
// try to allocate one at the very end
	if ( hunk_base + hunk_size - hunk_high_used - (byte *)c_new >= size)
	{
		memset (c_new, 0, sizeof(*c_new));
		c_new->size = size;
		
		c_new->next = &cache_head;
		c_new->prev = cache_head.prev;
		cache_head.prev->next = c_new;
		cache_head.prev = c_new;
		
		Cache_MakeLRU (c_new);

		m_CacheSystem = c_new;
		return c_new;
	}
	
	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void CMemCache::Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		g_MemCache->Cache_Free ( cache_head.next->user );	// reclaim the space
}


/*
============
Cache_Print

============
*/
void CMemCache::Cache_Print (void)
{
	CMemCacheSystem	*cd;

	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		Con_Printf ("%8i : %s\n", cd->size, cd->name);
	}
}

flush_cache_callback CMemCache::Cache_Flush_Callback()
{
	Cache_Flush();
	return 0;
}

/*
============
Cache_Report

============
*/
void CMemCache::Cache_Report (void)
{
	Con_DPrintf ("%4.1f megabyte data cache\n", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Compact

============
*/
void CMemCache::Cache_Compact (void)
{
}

/*
============
Cache_Init

============
*/
void CMemCache::Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	Cmd_AddCommand ("flush", Cache_Flush_Callback());
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void CMemCache::Cache_Free (cache_user_t *c)
{
	CMemCacheSystem	*cs;

	if (!c->data)
		Sys_Error ("Cache_Free: not allocated");

	cs = ((CMemCacheSystem *)c->data) - 1;

	if (cs)
	{
		cs->prev->next = cs->next;
		cs->next->prev = cs->prev;
		cs->next = cs->prev = NULL;
	}

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}



/*
==============
Cache_Check
==============
*/
void* CMemCache::Cache_Check (cache_user_t *c)
{
	CMemCacheSystem	*cs;

	if (!c->data)
		return NULL;

	cs = ((CMemCacheSystem *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);
	
	return c->data;
}


/*
==============
Cache_Alloc
==============
*/
void* CMemCache::Cache_Alloc (cache_user_s *c, int size, char *name)
{
	CMemCacheSystem	*cs;

	if (c->data)
		Sys_Error ("Cache_Alloc: allready allocated");
	
	if (size <= 0)
		Sys_Error ("Cache_Alloc: size %i", size);

	size = (size + sizeof(CMemCacheSystem) + 15) & ~15;

// find memory for it	
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			strncpy (cs->name, name, sizeof(cs->name)-1);
			c->data = (void *)(cs+1);
			cs->user = c;
			break;
		}
	
	// free the least recently used cahedat
		if (cache_head.lru_prev == &cache_head)
			Sys_Error ("Cache_Alloc: out of memory");
													// not enough memory at all
		Cache_Free ( cache_head.lru_prev->user );
	} 
	
	return Cache_Check (c);
}

//============================================================================

CMemCache::CMemCache() : mainzone(NULL), m_CacheSystem(NULL)
{
	if (!m_CacheSystem)
	{
		m_CacheSystem = new CMemCacheSystem;
	}
	if (!mainzone)
	{
		mainzone = new CMemZone;
	}
}

/*
========================
Memory_Init
========================
*/
void CMemCache::Memory_Init (void *buf, int size)
{
	int p;
	int zonesize = DYNAMIC_SIZE;

	hunk_base = static_cast<byte*>(buf);
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	g_MemCache->Cache_Init ();
	p = common->COM_CheckParm ("-zone");
	if (p)
	{
		if (p < common->com_argc-1)
			zonesize = Q_atoi (common->com_argv[p+1]) * 1024;
		else
			Sys_Error ("Memory_Init: you must specify a size in KB after -zone");
	}
	mainzone = static_cast<CMemZone*>(g_MemCache->Hunk_AllocName (zonesize, "zone" ));
	mainzone->Z_ClearZone (mainzone, zonesize);
}

CMemCacheSystem::CMemCacheSystem() : size(0), user(NULL), name(""), prev(NULL), next(NULL), lru_prev(NULL), lru_next(NULL)
{
}
