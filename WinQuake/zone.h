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

	int		size = 0;           // including the header and possibly tiny fragments
	int     tag = 0;            // a tag of 0 is a free block
	int     id = ZONEID;        		// should be ZONEID
	CMemBlock<unsigned char>* next, *prev = NULL;
	int		pad = 0;			// pad to 64 bit boundary

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

struct cache_user_t
{
	void* data = 0;
};

#define CACHENAME_LEN	32
class CMemCacheSystem
{
public:

	CMemCacheSystem();

	int			size;		// including this header
	cache_user_t* user;
	char			name[CACHENAME_LEN];
	class CMemCacheSystem* prev, * next;
	class CMemCacheSystem* lru_prev, * lru_next;	// for LRU flushing
};

// CMemCache* g_MemCache;

static CMemCacheSystem	cache_head;

typedef void (*flush_cache_callback)(void);

class CMemZone
{
public:
	int		size;		// total bytes malloced, including header
	CMemBlock<unsigned char>	blocklist;		// start / end cap for linked list
	CMemBlock<unsigned char>* rover;

	void Z_Free(void* ptr);
	void* Z_Malloc(int size);			// returns 0 filled memory
	void* Z_TagMalloc(int size, int tag);

	void Z_DumpHeap(void);
	void Z_CheckHeap(void);
	int Z_FreeMemory(void);

	void Z_ClearZone(CMemZone* zone, int size);
	void Z_Print(CMemZone* zone);
};

//CMemZone* mainzone;

class CMemCache
{
public:

	CMemCache();

	CMemZone* mainzone;

	void Memory_Init(void* buf, int size);

	void* Hunk_Alloc(int size);
	void Hunk_Print(bool all);
	// returns 0 filled memory
	void* Hunk_AllocName(int size, const char* name);

	void* Hunk_HighAllocName(int size, const char* name);

	int	Hunk_LowMark(void);
	void Hunk_FreeToLowMark(int mark);

	int	Hunk_HighMark(void);
	void Hunk_FreeToHighMark(int mark);

	void* Hunk_TempAlloc(int size);

	void Cache_Move(CMemCacheSystem* c);

	void Hunk_Check(void);

	void Cache_FreeHigh(int new_high_hunk);
	void Cache_FreeLow(int new_low_hunk);

	void Cache_MakeLRU(CMemCacheSystem* cs);
	void Cache_UnlinkLRU(CMemCacheSystem* cs);

	static void Cache_Flush(void);
	static flush_cache_callback Cache_Flush_Callback();
	void Cache_Print(void);

	void* Cache_Check(cache_user_t* c);
	// returns the cached data, and moves to the head of the LRU list
	// if present, otherwise returns NULL

	void Cache_Init(void);

	void Cache_Free(cache_user_t* c);

	void* Cache_Alloc(cache_user_t* c, int size, char* name);
	// Returns NULL if all purgable data was tossed and there still
	// wasn't enough room.

	CMemCacheSystem* Cache_TryAlloc(int size, bool nobottom);

	void Cache_Report(void);
	void Cache_Compact(void);

private:

	CMemCacheSystem* m_CacheSystem;
};

extern CMemCache* g_MemCache;

template<class T, class I>
CMemBlock<T, I>::CMemBlock(int growSize, int allocationCount) : m_pMemory(0), size(allocationCount), m_growSize(growSize)
{
	if (m_growSize < 0)
		return;

	if (size)
	{
		m_pMemory = (T*)calloc(size, sizeof(T));	// Missi: allocate if we specified a size (3/3/2022)
	}
	//else
	//{
	//	m_pMemory = (T*)calloc(1, sizeof(T));	// Missi: allocate anyway even if a size wasn't specified (3/3/2022)
	//}
}

template<class T, class I>
CMemBlock<T, I>::CMemBlock(T* memory, int numelements) : m_pMemory(memory), size(numelements)
{
	m_growSize = -1;
}

template<class T, class I>
CMemBlock<T, I>::CMemBlock(const T* memory, int numelements) : m_pMemory(memory), size(numelements)
{
	m_growSize = -2;
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