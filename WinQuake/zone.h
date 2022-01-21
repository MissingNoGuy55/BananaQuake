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

template<class T, class I = int>
class CMemBlock
{
public:

	CMemBlock(int startSize = 0, int growSize = 1);
	CMemBlock(T* memory, int numelements);
	CMemBlock(const T* memory, int numelements);

	int		size;           // including the header and possibly tiny fragments
	int     tag;            // a tag of 0 is a free block
	int     id;        		// should be ZONEID
	CMemBlock<unsigned char> *next, *prev;
	int		pad;			// pad to 64 bit boundary

	T* Base();
	const T* Base() const;

	// element access
	T& operator[](I i);
	const T& operator[](I i) const;

	T& operator=(int i) const;

	T& Element(I i);
	const T& Element(I i) const;

private:

	T* m_Memory;
	int	m_growSize;
	int m_Count;

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

typedef struct cache_user_s
{
	void* data;
} cache_user_t;

class CCacheSystem
{
public:
	int						size;		// including this header
	cache_user_t* user;
	char					name[16];
	class CCacheSystem *prev, *next;
	class CCacheSystem *lru_prev, *lru_next;	// for LRU flushing	
};

// CMemCache* g_MemCache;

static CCacheSystem	cache_head;

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

	CMemZone* mainzone;

	void Memory_Init(void* buf, int size);

	void* Hunk_Alloc(int size);
	void Hunk_Print(bool all);
	// returns 0 filled memory
	void* Hunk_AllocName(int size, char* name);

	void* Hunk_HighAllocName(int size, char* name);

	int	Hunk_LowMark(void);
	void Hunk_FreeToLowMark(int mark);

	int	Hunk_HighMark(void);
	void Hunk_FreeToHighMark(int mark);

	void* Hunk_TempAlloc(int size);

	void Cache_Move(CCacheSystem* c);

	void Hunk_Check(void);

	void Cache_FreeHigh(int new_high_hunk);
	void Cache_FreeLow(int new_low_hunk);

	void Cache_MakeLRU(CCacheSystem* cs);
	void Cache_UnlinkLRU(CCacheSystem* cs);

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

	CCacheSystem* Cache_TryAlloc(int size, bool nobottom);

	void Cache_Report(void);
	void Cache_Compact(void);

private:

	CCacheSystem* m_CacheSystem;
};

extern CMemCache* g_MemCache;

template<class T, class I>
inline CMemBlock<T, I>::CMemBlock(int startSize, int growSize)
{
	size = 0;
	tag = 1;
	next = prev = NULL;
	pad = 0;
	id = 0;
	m_Memory = 0;
	if (startSize > 0)
	{
		m_Memory = (T*)malloc(startSize * sizeof(T));
	}
}

template<class T, class I>
inline CMemBlock<T, I>::CMemBlock(T* memory, int numelements)
{
	m_Memory = memory;
}

template<class T, class I>
inline T* CMemBlock<T, I>::Base()
{
	return m_Memory;
}

template<class T, class I>
inline const T* CMemBlock<T, I>::Base() const
{
	return m_Memory;
}

template<class T, class I>
inline T& CMemBlock<T, I>::operator[](I i)
{
	return m_Memory[(Uint32)i];
}

template<class T, class I>
inline const T& CMemBlock<T, I>::operator[](I i) const
{
	return m_Memory[(Uint32)i];
}

template<class T, class I>
inline T& CMemBlock<T, I>::operator=(int i) const
{
	return m_Memory[i];
}

#endif