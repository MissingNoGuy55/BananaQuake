#pragma once

#include "quakedef.h"

/*
=============================================================================
* Missi: C++ vector wrapper
*
* This is basically a C++-like vector that can store anything.
*
* Do note that m_pBase is always a pointer, so if you make the vector store 
* pointers, it is, in reality, a double pointer.
* 
* With that being said, all elements are malloc'd and STAY IN MEMORY until
* you run RemoveEverything(), which will reset the vector back to zero-size
* and free all memory pertaining to elements. You will need to run this 
* function every time you want to clear the vector, or else memory will
* persist. DO NOT run memset on a vector as it will not clear the malloc'd
* elements! (6/22/2024)
=============================================================================
*/
template<typename T>
class CQVector
{
public:
	CQVector();
	~CQVector();

	//============================
	// Missi: Adding elements (6/13/2024)
	//============================

	void		AddToStart();
	void		AddToEnd();

	void		AddToEnd(const T& element);
	void		AddToEnd(T& element);
	void		AddToStart(const T& element);
	void		AddToStart(T& element);

	void		AddMultipleToStart(int num, T* element);
	void		AddMultipleToStart(int num, const T* element);
	void		AddMultipleToEnd(int num, T* element);
	void		AddMultipleToEnd(int num, const T* element);

	void		Expand(size_t size);
	void		Destroy(T* mem);
	void		Shrink();

	//============================
	// Missi: Removing elements (6/13/2024)
	//============================

	void		RemoveElement(int elem);

	//============================
	// Missi: Element management (6/13/2024)
	//============================

	T&			Element(int num);
	const T&	Element(int num) const;

	void		Clear();
	void		ShiftAllRight();
	void		ShiftMultipleRight(int num);
	const int	GetNumAllocated() const { return m_iAllocated; }
	T*			GetBase() { return m_pBase; }
	size_t		GetSize() const { return m_iSize; }

	T& operator[](int i);
	const T& operator[](int i) const;

	//============================
	// Missi: Private vars (6/13/2024)
	//============================
private:
	size_t		m_iSize;			// Missi: the amount of bytes allocated (6/13/2024)
	int			m_iAllocated;		// Missi: the amount of elements stored (6/13/2024)
	T*			m_pBase;			// Missi: the data itself. note that this is a double pointer if you store pointers with a vector (6/13/2024)
};

template<typename T>
CQVector<T>::CQVector()
{
	m_iSize = 0;
	m_iAllocated = 0;
	m_pBase = nullptr;
}

template<typename T>
CQVector<T>::~CQVector()
{
	m_iSize = 0;
	m_iAllocated = 0;
}

template<typename T>
T& CQVector<T>::operator[](int i)
{
	return m_pBase[i];
}

template<typename T>
const T& CQVector<T>::operator[](int i) const
{
	return m_pBase[i];
}

template<typename T>
void CQVector<T>::Expand(size_t size)
{
    m_pBase = (T*)realloc(m_pBase, size * sizeof(T));
    m_iSize = (size * sizeof(T));
    m_iAllocated = size;
}

template<typename T>
void CQVector<T>::Destroy(T* mem)
{
	mem->~T();

	memset(reinterpret_cast<void*>(mem), 0, sizeof(T));

	if (m_iSize > 0)
		m_iSize -= sizeof(T);

	m_iAllocated--;
}

template<typename T>
void CQVector<T>::Shrink()
{
	if (m_iSize != (m_iAllocated * sizeof(T)))
		m_iSize = (m_iAllocated * sizeof(T));

	m_pBase = (T*)realloc(m_pBase, m_iSize);
}

template<typename T>
void CQVector<T>::AddToEnd(T& element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* test = (T*)realloc(m_pBase, sizeof(T) * m_iSize);
		m_pBase = static_cast<T*>(test);
	}

	if (m_pBase)
	{
		memcpy(&m_pBase[m_iAllocated], element, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CQVector<T>::AddToStart()
{
	const T newElement = {};
	AddToStart(newElement);
	m_iSize += sizeof(T);
	m_iAllocated++;
}

template<typename T>
void CQVector<T>::AddToEnd()
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, sizeof(T) * m_iSize);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		T newElem = (T)calloc(1, sizeof(T));
		memcpy(&m_pBase[m_iAllocated], &newElem, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CQVector<T>::AddToEnd(const T& element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T));
	else
	{
		T* newMem = (T*)realloc(m_pBase, sizeof(T) * m_iSize);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		memcpy(&m_pBase[m_iAllocated], element, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CQVector<T>::AddToStart(T& element)
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
		memcpy(&m_pBase[m_iAllocated], element, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CQVector<T>::AddToStart(const T& element)
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
		element = new T;
		memcpy(&m_pBase[0], element, sizeof(T));
		m_iSize += sizeof(T);
		m_iAllocated++;
	}
}

template<typename T>
void CQVector<T>::AddMultipleToStart(int num, T* element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T) * num);
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
			element = new T;
			memcpy(&m_pBase[0], element[m_iAllocated], sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CQVector<T>::AddMultipleToStart(int num, const T* element)
{
	if (!m_pBase)
		m_pBase = (T*)malloc(sizeof(T) * num);
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
			element = new T;
			memcpy(&m_pBase[0], element[m_iAllocated], sizeof(T));
			m_iSize += sizeof(T);
			m_iAllocated++;
		}
	}
}

template<typename T>
void CQVector<T>::AddMultipleToEnd(int num, T* element)
{
	if (!m_pBase)
	{
		m_pBase = (T*)malloc(sizeof(T) * num);
		m_iSize += sizeof(T) * num;
	}
	else
	{
		T* newMem = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) * num);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			memcpy(&m_pBase[m_iAllocated], &element[m_iAllocated], sizeof(T));
			m_iAllocated++;
		}
	}
}

template<typename T>
void CQVector<T>::AddMultipleToEnd(int num, const T* element)
{
	if (!m_pBase)
	{
		m_pBase = (T*)malloc(sizeof(T) * num);
		m_iSize += sizeof(T) * num;
	}
	else
	{
		T* newMem = (T*)realloc(m_pBase, (sizeof(T) * m_iSize) * num);
		m_pBase = static_cast<T*>(newMem);
	}

	if (m_pBase)
	{
		for (int i = 0; i < num; i++)
		{
			memcpy(&m_pBase[m_iAllocated], &element[m_iAllocated], sizeof(T));
			m_iAllocated++;
		}
	}
}

template<typename T>
void CQVector<T>::RemoveElement(int elem)
{
	Destroy(&Element(elem));
}

template<typename T>
T& CQVector<T>::Element(int num)
{
	return m_pBase[num];
}

template<typename T>
const T& CQVector<T>::Element(int num) const
{
	return m_pBase[num];
}

template<typename T>
void CQVector<T>::Clear()
{
    free(m_pBase);
    m_pBase = nullptr;

    m_iSize = 0;
    m_iAllocated = 0;
}

template<typename T>
void CQVector<T>::ShiftAllRight()
{
	if ((m_iAllocated > 0))
		memmove(&m_pBase[m_iAllocated], &m_pBase[0], m_iAllocated * sizeof(T));
}

template<typename T>
void CQVector<T>::ShiftMultipleRight(int num)
{
	if ((m_iAllocated > 0))
		memmove(&m_pBase[m_iAllocated + num], &m_pBase[0], (m_iAllocated * sizeof(T)) * num);
}
