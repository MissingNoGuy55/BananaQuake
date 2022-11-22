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
/*
 memory allocation


H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

#ifndef ZONE_H
#define ZONE_H

#define	DYNAMIC_SIZE	0xc000

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

template<class T, class I = int>
class CMemBlock
{
public:

	CMemBlock(int growSize = 0, int startSize = 0);
	CMemBlock(T* memory, int numelements);
	CMemBlock(const T* memory, int numelements);
	~CMemBlock();

	int		size;           // including the header and possibly tiny fragments
	int     tag;            // a tag of 0 is a free block
	int     id;        		// should be ZONEID
	CMemBlock<unsigned char>* next, *prev;
	int		pad;			// pad to 64 bit boundary

	T* Base();
	const T* Base() const;

	void Purge();
	void Purge(int numElements);

	// element access
	T& operator[](I i);
	const T& operator[](I i) const;

	T& operator=(int i) const;

	T& Element(I i);
	const T& Element(I i) const;

	int NumAllocated();
	void Grow(int num);

	bool IsExternallyAllocated();

	int CalcNewAllocationCount(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem);

	void Init();

private:

	T* m_pMemory;
	int	m_growSize;
	// int m_Count;

};


//
//class CMemBlock
//{
//public:
//
//	CMemBlock();
//
//	int		size;           // including the header and possibly tiny fragments
//	int     tag;            // a tag of 0 is a free block
//	int     id;        		// should be ZONEID
//	CMemBlock *next, *prev;
//	int		pad;			// pad to 64 bit boundary
//};
//

#define CACHENAME_LEN	32
class CMemCacheSystem
{
public:

	CMemCacheSystem();

	int			size;		// including this header

	template<typename T>
	static cache_user_s<T>* user;

	char			name[CACHENAME_LEN];
	class CMemCacheSystem* prev, * next;
	class CMemCacheSystem* lru_prev, * lru_next;	// for LRU flushing
};

// CMemCache* g_MemCache;

static CMemCacheSystem	cache_head;

template<typename T>
cache_user_s<T>* CMemCacheSystem::user = {};

typedef void (*flush_cache_callback)(void);

class CMemZone
{
public:

	CMemZone();
	~CMemZone();

	int		size;		// total bytes malloced, including header
	CMemBlock<unsigned char>	blocklist;		// start / end cap for linked list
	CMemBlock<unsigned char>* rover;

	void Z_Free(void* ptr);

	template<typename T>
	T* Z_Malloc(int size);			// returns 0 filled memory
	void* Z_TagMalloc(int size, int tag);

	void Z_DumpHeap(void);
	void Z_CheckHeap(void);
	int Z_FreeMemory(void);

	void Z_ClearZone(CMemZone* zone, int size);
	void Z_Print(CMemZone* zone);
};

/*
========================
Z_Malloc
========================
*/
template<typename T>
T* CMemZone::Z_Malloc(int size)
{
	T* buf = 0;

	Z_CheckHeap();	// DEBUG
	buf = (T*)Z_TagMalloc(size, 1);
	if (!buf)
		Sys_Error("Z_Malloc: failed on allocation of %i bytes", size);
	Q_memset(buf, 0, size);

	if (zone_debug.value > 0)
	{
		Z_Print(this);
	}

	return buf;
}

//CMemZone* mainzone;

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[24];
} hunk_t;

extern byte* hunk_base;
extern int		hunk_size;

extern int		hunk_low_used;
extern int		hunk_high_used;

extern bool	hunk_tempactive;
extern int		hunk_tempmark;

class CMemCache
{
public:

	CMemCache();

	CMemZone* mainzone;

	void Memory_Init(void* buf, int size);

	template<typename T>
	T* Hunk_Alloc(int size);

	void Hunk_Print(bool all);
	// returns 0 filled memory

	template<typename T>
	T* Hunk_AllocName(int size, const char* name);

	template<typename T>
	T* Hunk_HighAllocName(int size, const char* name);

	int	Hunk_LowMark(void);
	void Hunk_FreeToLowMark(int mark);

	int	Hunk_HighMark(void);
	void Hunk_FreeToHighMark(int mark);

	template<typename T>
	T* Hunk_TempAlloc(int size);

	template<typename T>
	void Cache_Move(CMemCacheSystem* c);

	void Hunk_Check(void);

	template<typename T>
	void Cache_FreeHigh(int new_high_hunk);

	template<typename T>
	void Cache_FreeLow(int new_low_hunk);

	static void Cache_MakeLRU(CMemCacheSystem* cs);
	static void Cache_UnlinkLRU(CMemCacheSystem* cs);

	template<typename T>
	static void Cache_Flush(void);

	static flush_cache_callback Cache_Flush_Callback();
	void Cache_Print(void);

	template<typename T>
	static T* Cache_Check(cache_user_s<T>* c);
	// returns the cached data, and moves to the head of the LRU list
	// if present, otherwise returns NULL

	void Cache_Init(void);

	template<typename T>
	void Cache_Free(cache_user_s<T>* c);

	template<typename T>
	T* Cache_Alloc(cache_user_s<T>* c, int size, char* name);
	// Returns NULL if all purgable data was tossed and there still
	// wasn't enough room.

	template<typename T>
	CMemCacheSystem* Cache_TryAlloc(int size, bool nobottom);

	void Cache_Report(void);
	void Cache_Compact(void);

private:

	CMemCacheSystem* m_CacheSystem;
};

/*
==============
Cache_Check
==============
*/
template<typename T>
T* CMemCache::Cache_Check(cache_user_s<T>* c)
{
	CMemCacheSystem* cs;

	if (!c->data)
		return NULL;

	cs = ((CMemCacheSystem*)c->data) - 1;

	// move to head of LRU
	Cache_UnlinkLRU(cs);
	Cache_MakeLRU(cs);

	return c->data;
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
template<typename T>
void CMemCache::Cache_Free(cache_user_s<T>* c)
{
	CMemCacheSystem* cs;

	if (!c->data)
		Sys_Error("Cache_Free: not allocated");

	cs = ((CMemCacheSystem*)c->data) - 1;

	if (cs)
	{
		cs->prev->next = cs->next;
		cs->next->prev = cs->prev;
		cs->next = cs->prev = NULL;
	}

	c->data = NULL;

	Cache_UnlinkLRU(cs);
}


/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
template<typename T>
void CMemCache::Cache_FreeHigh(int new_high_hunk)
{
	CMemCacheSystem* c = NULL, * prev = NULL;

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
				Cache_Free<T>(c->user<T>);	// didn't move out of the way
			else
			{
				Cache_Move<T>(c);	// try to move it
				prev = c;
			}
		}
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
template<typename T>
void CMemCache::Cache_FreeLow(int new_low_hunk)
{
	CMemCacheSystem* c;

	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ((byte*)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move<T>(c);	// reclaim the space
	}
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
template<typename T>
CMemCacheSystem* CMemCache::Cache_TryAlloc(int size, bool nobottom)
{
	CMemCacheSystem* cs, * c_new;

	// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			Sys_Error("Cache_TryAlloc: %i is greater then free hunk", size);

		c_new = (CMemCacheSystem*)(hunk_base + hunk_low_used);
		memset(c_new, 0, sizeof(*c_new));
		c_new->size = size;

		cache_head.prev = cache_head.next = c_new;
		c_new->prev = c_new->next = &cache_head;

		Cache_MakeLRU(c_new);
		m_CacheSystem = c_new;
		return c_new;
	}

	// search from the bottom up for space

	c_new = (CMemCacheSystem*)(hunk_base + hunk_low_used);
	cs = cache_head.next;

	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ((byte*)cs - (byte*)c_new >= size)
			{	// found space
				memset(c_new, 0, sizeof(*c_new));
				c_new->size = size;

				c_new->next = cs;
				c_new->prev = cs->prev;
				cs->prev->next = c_new;
				cs->prev = c_new;

				Cache_MakeLRU(c_new);

				m_CacheSystem = c_new;
				return c_new;
			}
		}

		// continue looking		
		c_new = (CMemCacheSystem*)((byte*)cs + cs->size);
		cs = cs->next;

	} while (cs != &cache_head);

	// try to allocate one at the very end
	if (hunk_base + hunk_size - hunk_high_used - (byte*)c_new >= size)
	{
		memset(c_new, 0, sizeof(*c_new));
		c_new->size = size;

		c_new->next = &cache_head;
		c_new->prev = cache_head.prev;
		cache_head.prev->next = c_new;
		cache_head.prev = c_new;

		Cache_MakeLRU(c_new);

		m_CacheSystem = c_new;
		return c_new;
	}

	return NULL;		// couldn't allocate
}

/*
==============
Cache_Alloc
==============
*/
template<typename T>
T* CMemCache::Cache_Alloc(cache_user_s<T>* c, int size, char* name)
{
	CMemCacheSystem* cs;

	if (c->data)
		Sys_Error("Cache_Alloc: allready allocated");

	if (size <= 0)
		Sys_Error("Cache_Alloc: size %i", size);

	size = (size + sizeof(CMemCacheSystem) + 15) & ~15;

	// find memory for it	
	while (1)
	{
		cs = Cache_TryAlloc<T>(size, false);
		if (cs)
		{
			strncpy(cs->name, name, sizeof(cs->name) - 1);
			c->data = (T*)(cs + 1);
			cs->user<T> = c;
			break;
		}

		// free the least recently used cahedat
		if (cache_head.lru_prev == &cache_head)
			Sys_Error("Cache_Alloc: out of memory");
		// not enough memory at all
		Cache_Free<T>(cache_head.lru_prev->user<T>);
	}

	return Cache_Check<T>(c);
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
template<typename T>
void CMemCache::Cache_Flush(void)
{
	while (cache_head.next != &cache_head)
		g_MemCache->Cache_Free<T>(cache_head.next->user<T>);	// reclaim the space
}

/*
===========
Cache_Move
===========
*/
template<typename T>
void CMemCache::Cache_Move(CMemCacheSystem* c)
{
	CMemCacheSystem* c_new;

	// we are clearing up space at the bottom, so only allocate it late
	c_new = Cache_TryAlloc<T>(c->size, true);
	if (c_new)
	{
		//		Con_Printf ("cache_move ok\n");

		Q_memcpy(c_new + 1, c + 1, c->size - sizeof(CMemCacheSystem));
		c_new->user<T> = c->user<T>;
		Q_memcpy(c_new->name, c->name, sizeof(c_new->name));
		Cache_Free(c->user<T>);
		c_new->user<T>->data = (T*)(c_new + 1);
	}
	else
	{
		//		Con_Printf ("cache_move failed\n");

		Cache_Free(c->user<T>);		// tough luck...
	}
}

/*===================
Hunk_Alloc

Missi: Allocates a block of memory on the hunk. Usually used for things like textures and models. Use CMemZone if you need string or volatile memory allocation.

This function exists just for brevity; it calls Hunk_AllocName with a const char parameter of "unknown" (3/8/2022)
===================*/
template<typename T>
T* CMemCache::Hunk_Alloc(int size)
{
	return Hunk_AllocName<T>(size, "unknown");
}

/*===================
Hunk_AllocName

Missi: Allocates a named block of memory on the hunk. Castable to any type. Use only for things like models and textures. If you need small, possibly volatile memory, use CMemZone::Z_TagMalloc instead.
===================*/
template<typename T>
T* CMemCache::Hunk_AllocName(int size, const char* name)
{
	hunk_t* h;

#ifdef PARANOID
	Hunk_Check();
#endif

	if (size < 0)
		Sys_Error("Hunk_Alloc: bad size: %i", size);

	size = sizeof(hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
		Sys_Error("Hunk_Alloc: failed on %i bytes", size);

	h = (hunk_t*)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow<T>(hunk_low_used);

	memset(h, 0, size);

	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy(h->name, name, 24);

	return (T*)(h + 1);
}

/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
template<typename T>
T* CMemCache::Hunk_TempAlloc(int size)
{
	T* buf = 0;

	size = (size + 15) & ~15;

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark(hunk_tempmark);
		hunk_tempactive = false;
	}

	hunk_tempmark = Hunk_HighMark();

	buf = Hunk_HighAllocName<T>(size, "temp");

	hunk_tempactive = true;

	return buf;
}

/*
===================
Hunk_HighAllocName
===================
*/
template<typename T>
T* CMemCache::Hunk_HighAllocName(int size, const char* name)
{
	hunk_t* h;

	if (size < 0)
		Sys_Error("Hunk_HighAllocName: bad size: %i", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark(hunk_tempmark);
		hunk_tempactive = false;
	}

#ifdef PARANOID
	Hunk_Check();
#endif

	size = sizeof(hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		Con_Printf("Hunk_HighAlloc: failed on %i bytes\n", size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh<T>(hunk_high_used);

	h = (hunk_t*)(hunk_base + hunk_size - hunk_high_used);

	memset(h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	Q_strncpy(h->name, name, 8);

	return (T*)(h + 1);
}

extern CMemCache* g_MemCache;

template<class T, class I>
CMemBlock<T, I>::CMemBlock(int growSize, int allocationCount) : m_pMemory(0), size(allocationCount), m_growSize(growSize), id(ZONEID), tag(0), prev(NULL), next(NULL), pad(0)
{
	if (m_growSize < 0)
		return;

	if (size)
	{
		m_pMemory = (T*)calloc(size, sizeof(T));	// Missi: allocate if we specified a size (3/3/2022)
	}
	else
	{
		m_pMemory = (T*)calloc(1, sizeof(T));	// Missi: allocate anyway even if a size wasn't specified (3/3/2022)
	}
}

template<class T, class I>
CMemBlock<T, I>::CMemBlock(T* memory, int numelements) : m_pMemory(memory), size(numelements), id(ZONEID), tag(0), prev(NULL), next(NULL), pad(0)
{
	m_growSize = -1;
}

template<class T, class I>
CMemBlock<T, I>::CMemBlock(const T* memory, int numelements) : m_pMemory(memory), size(numelements), id(ZONEID), tag(0), prev(NULL), next(NULL)
{
	m_growSize = -2;
}

template<class T, class I>
inline void CMemBlock<T, I>::Init()
{
	m_pMemory = (T*)malloc(sizeof(T));
	size = 0;
	tag = 0;
	id = ZONEID;
	next = prev = NULL;
	pad = 0;
	m_growSize = 0;
}

template<class T, class I>
inline CMemBlock<T, I>::~CMemBlock()
{
	Purge();
}

template<class T, class I>
inline T* CMemBlock<T, I>::Base()
{
	return m_pMemory;
}

template<class T, class I>
inline const T* CMemBlock<T, I>::Base() const
{
	return m_pMemory;
}

template<class T, class I>
inline T& CMemBlock<T, I>::Element(I i)
{
	return m_pMemory[(Uint32)i];
}

template<class T, class I>
inline const T& CMemBlock<T, I>::Element(I i) const
{
	return m_pMemory[(Uint32)i];
}

template<class T, class I>
inline void CMemBlock<T, I>::Purge()
{
	if (m_pMemory)
	{
		free(m_pMemory);
		m_pMemory = 0;
	}
	size = 0;
}

template<class T, class I>
inline T& CMemBlock<T, I>::operator[](I i)
{
	return m_pMemory[(Uint32)i];
}

template<class T, class I>
inline const T& CMemBlock<T, I>::operator[](I i) const
{
	return m_pMemory[(Uint32)i];
}

template<class T, class I>
inline T& CMemBlock<T, I>::operator=(int i) const
{
	return m_pMemory[i];
}

template<class T, class I>
inline int CMemBlock<T, I>::NumAllocated()
{
	return size;
}

template<class T, class I>
void CMemBlock<T, I>::Grow(int num)
{

	if (IsExternallyAllocated())
		return;

	if (!num)
		Con_Printf("CMemBlock::Grow called with 0 grow size!\n");

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	int nAllocationRequested = size + num;

	int nNewAllocationCount = CalcNewAllocationCount(size, m_growSize, nAllocationRequested, sizeof(T));

	// if m_nAllocationRequested wraps index type I, recalculate
	if ((int)(I)nNewAllocationCount < nAllocationRequested)
	{
		if ((int)(I)nNewAllocationCount == 0 && (int)(I)(nNewAllocationCount - 1) >= nAllocationRequested)
		{
			--nNewAllocationCount; // deal w/ the common case of m_nAllocationCount == MAX_USHORT + 1
		}
		else
		{
			if ((int)(I)nAllocationRequested != nAllocationRequested)
			{
				// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
				return;
			}
			while ((int)(I)nNewAllocationCount < nAllocationRequested)
			{
				nNewAllocationCount = (nNewAllocationCount + nAllocationRequested) / 2;
			}
		}
	}

	size = nNewAllocationCount;

	if (m_pMemory)
	{
		m_pMemory = static_cast<T*>(realloc(m_pMemory, size * sizeof(T)));

		if (!m_pMemory)
		{
			Sys_Error("CMemBlock::Grow: m_pMemory returned NULL\n");
			return;
		}
	}
	else
	{
		m_pMemory = static_cast<T*>(malloc(size * sizeof(T)));
	}
}

template<class T, class I>
inline bool CMemBlock<T, I>::IsExternallyAllocated()
{
	return (m_growSize < 0);
}

template<class T, class I>
inline int CMemBlock<T, I>::CalcNewAllocationCount(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem)
{
	if (nGrowSize)
	{
		nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	}
	else
	{
		if (!nAllocationCount)
		{
			// Compute an allocation which is at least as big as a cache line...
			nAllocationCount = (31 + nBytesItem) / nBytesItem;
		}

		while (nAllocationCount < nNewSize)
		{
#ifndef _X360
			nAllocationCount *= 2;
#else
			int nNewAllocationCount = (nAllocationCount * 9) / 8; // 12.5 %
			if (nNewAllocationCount > nAllocationCount)
				nAllocationCount = nNewAllocationCount;
			else
				nAllocationCount *= 2;
#endif
		}
	}

	return nAllocationCount;
}

template<class T, class I>
inline void CMemBlock<T, I>::Purge(int numElements)
{
	if (numElements > size)
	{
		return;
	}

	// If we have zero elements, simply do a purge:
	if (numElements == 0)
	{
		Purge();
		return;
	}

	if (IsExternallyAllocated())
	{
		// Can't shrink a buffer whose memory was externally allocated, fail silently like purge 
		return;
	}

	// If the number of elements is the same as the allocation count, we are done.
	if (numElements == size)
	{
		return;
	}


	if (!m_pMemory)
	{
		return;
	}

	size = numElements;

	m_pMemory = (T*)realloc(m_pMemory, size * sizeof(T));
}

#endif