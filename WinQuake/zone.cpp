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
template<typename T>
extern CMemCacheSystem<T>* test_head;

template<typename T>
CMemCacheSystem<T>* test_head = new CMemCacheSystem<T>;

template<typename T>
CMemCacheSystem<T>		cache_head = *test_head<T>;
*/

byte* hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

bool	hunk_tempactive;
int		hunk_tempmark;

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
===============================================================================

CACHE MEMORY

===============================================================================
*/

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

CMemCache::CMemCache() : mainzone(NULL)
{
	if (!mainzone)
	{
		mainzone = new CMemZone;
	}
}

//============================================================================
