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

#pragma once

#include "pr_comp.h"			// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs

extern const char* PR_GetString(int num);	// Missi: brevity function from QuakeSpasm (11/24/2022)
											// Missi: moved up here because of usage in vector functions below (6/13/2024)

extern void Con_Warning(const char* fmt, ...);

constexpr size_t MAX_PROG_VECTOR_ALLOC = 32768U;
constexpr size_t MAX_PROG_VECTORS = 65535U;

#define	CONTENT_WATER	-3
#define CONTENT_SLIME	-4
#define CONTENT_LAVA	-5

#define FL_IMMUNE_WATER	131072
#define	FL_IMMUNE_SLIME	262144
#define FL_IMMUNE_LAVA	524288

#define	CHAN_VOICE	2
#define	CHAN_BODY	4

#define	ATTN_NORM	1

/*
=============================================================================
* Missi: prog vector
* 
* This is basically a CXX-like vector that can store anything as long as
* it's a malloc'd pointer of some kind.
* 
* The type of T is inherently void* all the time to allow storing multiple
* types within one class while respecting pointer sizes. This may change at 
* some point to be more in-line with how C++ vectors normally work.
* 
* With that being said, MAKE SURE YOU ARE ACCESSING THE RIGHT DATA TYPE FOR 
* THE CORRECT ELEMENT IN QC CODE IF YOU ARE TO MIX TYPES.
* 
* Do note as well that you can only allocate in bytes as high as 
* MAX_PROG_VECTOR will allow, per vector. Going any higher will invoke 
* a warning.
* 
* There are three places that elements are cleared in vectors in BananaQuake,
* CQuakeServer::SV_SpawnServer, CQuakeHost::Host_Shutdown and
* CQuakeHost::Host_ShutdownServer. (6/13/2024)
* 
* TODO: replace CQVector with an extended version of this at some point,
* as this is made from scratch (6/13/2024)
=============================================================================
*/
template<typename T>
class CProgVector
{
public:
	CProgVector();
	~CProgVector();

	//============================
	// Missi: Adding elements (6/13/2024)
	//============================

	void		AddToEnd(T& elem);
	void		AddToEnd(const T& elem);
	void		AddToStart(T& elem);
	void		AddToStart(const T& elem);

	void		AddMultipleToStart(int num, T& element);
	void		AddMultipleToStart(int num, const T& element);
	void		AddMultipleToEnd(int num, T& element);
	void		AddMultipleToEnd(int num, const T& element);

	//============================
	// Missi: Removing elements (6/13/2024)
	//============================

	void		RemoveElement(int elem);

	//============================
	// Missi: Element management (6/13/2024)
	//============================

	void		Clear();
	void		ShiftAllRight();
	void		ShiftMultipleRight(int num);
	string_t	GetName() const			{ return m_iName; }
	const int	GetNumAllocated() const { return m_iAllocated; }
	T*			GetBase() const			{ return m_pBase; }
	size_t		GetSize() const			{ return m_iSize; }

	//============================
	// Missi: Private vars (6/13/2024)
	//============================
private:
	size_t		m_iSize;			// Missi: the amount of bytes allocated (6/13/2024)
	string_t	m_iName;			// Missi: the string_t for progs to use (6/13/2024)
	int			m_iOffset;			// Missi: the offset for progs to use (6/13/2024)
	int			m_iAllocated;		// Missi: the amount of elements stored (6/13/2024)
	T*			m_pBase;			// Missi: the data itself. note that this is, in reality, a double pointer (6/13/2024)
};

template<typename T>
CProgVector<T>::CProgVector()
{
	m_iSize = 0;
	m_iName = 0;
	m_iOffset = 0;
	m_iAllocated = 0;
	m_pBase = nullptr;
}

template<typename T>
CProgVector<T>::~CProgVector()
{
	m_iSize = 0;
	m_iName = 0;
	m_iOffset = 0;
	m_iAllocated = 0;
}

template<typename T>
void CProgVector<T>::AddToEnd(T& elem)
{
	if (GetNumAllocated() >= MAX_PROG_VECTOR_ALLOC)
		return Con_Warning("WARNING: vector \"%s\" attempted to add an element that overflows MAX_PROG_VECTOR_ALLOC!\n", PR_GetString(GetName()));

	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* test = (T*)realloc(m_pBase, sizeof(T) * m_iSize);
		m_pBase = static_cast<T*>(test);
	}

	if (m_pBase)
	{
		m_pBase[m_iAllocated] = new T;
		memcpy(m_pBase[m_iAllocated], elem, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CProgVector<T>::AddToEnd(const T& elem)
{	
	if (GetNumAllocated() >= MAX_PROG_VECTOR_ALLOC)
		return Con_Warning("WARNING: vector \"%s\" attempted to add an element that overflows MAX_PROG_VECTOR_ALLOC!\n", PR_GetString(GetName()));

	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, sizeof(T) * m_iSize);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		m_pBase[m_iAllocated] = new T;
		memcpy(m_pBase[m_iAllocated], elem, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CProgVector<T>::AddToStart(T& elem)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, sizeof(T) * m_iSize + sizeof(T));
		m_pBase = static_cast<T*>(newMem);
		ShiftAllRight();
	}

	if (m_pBase)
	{
		m_pBase[m_iAllocated] = new T;
		memcpy(m_pBase[m_iAllocated], elem, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CProgVector<T>::AddToStart(const T& elem)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* test = (T*)realloc(m_pBase, sizeof(T) * m_iSize + sizeof(T));
		m_pBase = static_cast<T*>(test);
		ShiftAllRight();
	}

	if (m_pBase)
	{
		m_pBase[0] = new T;
		memcpy(m_pBase[0], elem, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CProgVector<T>::AddMultipleToStart(int num, T& element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* test = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) + (sizeof(T) * num));
		m_pBase = static_cast<T*>(test);
		ShiftMultipleRight(num);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			m_pBase[0] = new T;
			memcpy(m_pBase[0], element, sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CProgVector<T>::AddMultipleToStart(int num, const T& element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* test = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) + (sizeof(T) * num));
		m_pBase = static_cast<T*>(test);
		ShiftMultipleRight(num);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			m_pBase[0] = new T;
			memcpy(m_pBase[0], element, sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CProgVector<T>::AddMultipleToEnd(int num, T& element)
{
	if (GetNumAllocated() >= MAX_PROG_VECTOR_ALLOC)
		return Con_Warning("WARNING: vector \"%s\" attempted to add an element that overflows MAX_PROG_VECTOR_ALLOC!\n", PR_GetString(GetName()));

	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) * num);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			m_pBase[m_iAllocated + i] = new T;
			memcpy(m_pBase[m_iAllocated + i], element, sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CProgVector<T>::AddMultipleToEnd(int num, const T& element)
{
	if (GetNumAllocated() >= MAX_PROG_VECTOR_ALLOC)
		return Con_Warning("WARNING: vector \"%s\" attempted to add an element that overflows MAX_PROG_VECTOR_ALLOC!\n", PR_GetString(GetName()));

	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) * num);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			m_pBase[m_iAllocated+i] = new T;
			memcpy(m_pBase[m_iAllocated+i], element, sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CProgVector<T>::RemoveElement(int elem)
{
	if (!m_pBase)
		return Con_Warning("Tried to delete a null element from vector \"%s\"! (Element %d)\n", PR_GetString(m_iName), elem);

	delete m_pBase[elem];
	m_pBase[elem] = nullptr;
}

template<typename T>
void CProgVector<T>::Clear()
{
	delete[] m_pBase;
	delete this;
}

template<typename T>
void CProgVector<T>::ShiftAllRight()
{
	if ((m_iAllocated > 0))
		memmove(&m_pBase[m_iAllocated], &m_pBase[0], m_iAllocated * sizeof(T));
}

template<typename T>
void CProgVector<T>::ShiftMultipleRight(int num)
{
	if ((m_iAllocated > 0))
		memmove(&m_pBase[m_iAllocated + num], &m_pBase[0], (m_iAllocated * sizeof(T)) * num);
}

struct progvector_t
{
	progvector_t() {
	
	name = 0;
	ofs = 0;
	type = 0;
	data = nullptr;

	}

	string_t			name;
	unsigned short		ofs;
	unsigned short		type;
	CProgVector<void*>*	data;

};

//============================================================================

typedef union eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				_int;
	int				edict;
	progvector_t	cxxvector;
} eval_t;	

#define	MAX_ENT_LEAFS	16
typedef struct edict_s
{
	bool	free;
	link_t		area;				// linked to a division node or leaf
	
	int			num_leafs;
	short		leafnums[MAX_ENT_LEAFS];

	entity_state_t	baseline;
	
	float		freetime;			// sv->time when the object was freed
	entvars_t	v;					// C exported fields from progs
// other fields from progs come immediately after
} edict_t;
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

extern	dprograms_t		*progs;
extern	dfunction_t		*pr_functions;
extern	char			*pr_strings;
extern	int				pr_stringssize;
extern	ddef_t			*pr_globaldefs;
extern	ddef_t			*pr_fielddefs;
extern	dstatement_t	*pr_statements;
extern	globalvars_t	*pr_global_struct;
extern	float			*pr_globals;			// same as pr_global_struct
extern	CProgVector<void*>* pr_knownvectors;
extern	int		pr_vecsize;

extern int		pr_maxknownstrings;
extern int		pr_numknownstrings;
extern int		pr_numknownvectors;
extern progvector_t*	pr_vectors;
extern CQVector<const char*>	pr_knownstrings;

extern	int				pr_edict_size;	// in bytes

//============================================================================

void PR_Init ();

void PR_ExecuteProgram (func_t fnum);
void PR_LoadProgs ();

extern progvector_t* PR_GetCPPVector(int num);
extern int PR_SetEngineString(const char* s);

void PR_Profile_f ();

edict_t *ED_Alloc ();
void ED_Free (edict_t *ed);

string_t	ED_NewString (const char *string);
// returns a copy of the string allocated from the server's string heap

void ED_Print (edict_t *ed);
void ED_Write (cxxofstream *f, edict_t *ed);
const char *ED_ParseEdict (const char *data, edict_t *ent);

ddef_t *ED_FindGlobal (const char *name);

void ED_WriteGlobals (cxxofstream *f);
const char* ED_ParseGlobals (const char *data);

void ED_LoadFromFile (const char *data);

int PR_AllocString(int size, char** ptr);

//define EDICT_NUM(n) ((edict_t *)(sv->GetServerEdicts()+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv->GetServerEdicts())/pr_edict_size)

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t *e);

#define	NEXT_EDICT(e) ((edict_t *)( (byte *)e + pr_edict_size))

#define	EDICT_TO_PROG(e) ((byte *)e - (byte *)sv->GetServerEdicts())
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv->GetServerEdicts() + e))

//============================================================================

#define	G_FLOAT(o) (pr_globals[o])
#define	G_INT(o) (*(int *)&pr_globals[o])
#define	G_EDICT(o)		((edict_t *)((byte *)sv->GetServerEdicts() + *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o)		(PR_GetString(*(string_t *)&pr_globals[o]))
#define	G_STRING_MOD(o)	(*(string_t *)&pr_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])
#define G_CPPVECTOR(o) (ED_FindCPPVector(o))
#define G_VOID(o) ((void*)&pr_globals[o])

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
#define	E_STRING(e,o) (PR_GetString(*(string_t *)&((float*)&e->v)[o]))

extern	int		type_size[9];

typedef void (*builtin_t) ();
extern	builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern int		pr_argc;

extern	bool	pr_trace;
extern	dfunction_t	*pr_xfunction;
extern	int			pr_xstatement;

extern	unsigned short		pr_crc;

void PR_RunError (const char *error, ...);

void ED_PrintEdicts ();
void ED_PrintNum (int ent);

const char* _ED_ParseMultiManager(const char* data);

dfunction_t* ED_FindFunction(const char* name);
extern progvector_t* ED_FindCPPVector(const char* name);

eval_t* GetEdictFieldValue(edict_t* ed, const char* field);
edict_t* ED_FindEdict(const char* targetname);
const char* ED_FindEdictTextBlock(const char* targetname);
void GetEdictFieldValues(edict_t* ed, CQVector<eval_t*>* vec);
const char *PR_UglyValueString (int type, eval_t *val);
