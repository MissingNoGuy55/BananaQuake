#pragma once

#ifndef UTILS_H
#define UTILS_H

class CCacheSystem;

// CQVector -- a growable vector inspired by CUtlVector from Source.

template<class T, class S = CMemBlock<T> >
class CQVector
{
	typedef S CMemAllocator;
public:

	typedef T ElementType;
	
	CQVector(int startSize = 0, int growSize = 1);
	CQVector(T* memory, int startSize, int growSize = 1);
	~CQVector();

	CQVector<T, S>& operator=(const CQVector<T, S>& other);

	void Construct(T* element);
	void Destruct(T* element);

	T* Base() { return m_Memory.Base(); }
	const T* Base() const { return m_Memory.Base(); }

	T& operator[](int i);
	const T& operator[](int i) const;
	T& Element(int i);
	const T& Element(int i) const;
	T& Head();
	const T& Head() const;
	T& Tail();
	const T& Tail() const;
	T& Random();
	const T& Random() const;

	void SetCount(int count);

	void ShiftElementsRight(int elem, int num);
	void ShiftElementsLeft(int elem, int num);

	int AddToHead();
	int AddToTail();

	int AddToHead(const T& src);
	int AddToTail(const T& src);

	// Adds multiple elements, uses default constructor
	int AddMultipleToHead(int num);
	int AddMultipleToTail(int num, const T* pToCopy = NULL);
	int InsertMultipleBefore(int elem, int num, const T* pToCopy = NULL);	// If pToCopy is set, then it's an array of length 'num' and
	int InsertMultipleAfter(int elem, int num);
	void RemoveAll();				// doesn't deallocate memory

	int InsertAfter(int elem);
	int InsertBefore(int elem);

	int InsertBefore(int elem, const T& src);
	int InsertAfter(int elem, const T& src);

	// Returns the number of elements in the vector
	int Count() const;

	void GrowVector(int num = 1);

	// Is the vector empty?
	inline bool IsEmpty(void) const
	{
		return (Count() == 0);
	}

	inline void RefreshElements()
	{
		m_pElements = Base();
		Con_Printf("Test\n");
	}

//	const CCacheSystem* m_Cache;

	CMemAllocator m_Memory;

	T* m_pElements; // variable-sized

	int vecSize;

private:

	CQVector(CQVector const& vec);

};

// template<typename T, class S> CQVector<byte>::CQVector(byte);

template<class T, class S>
inline CQVector<T, S>::CQVector(int startSize, int growSize) : m_Memory(startSize, growSize)
{
	vecSize = startSize;
	RefreshElements();
}

template<class T, class S>
inline CQVector<T, S>::CQVector(T* memory, int startSize, int growSize)
{
	vecSize = startSize;
	RefreshElements();
}

template<class T, class S>
inline CQVector<T, S>::~CQVector()
{
}

template<class T, class S>
inline CQVector<T, S>& CQVector<T, S>::operator=(const CQVector<T, S>& other)
{
	int nCount = other.Count();
	vecSize = nCount;
	for (int i = 0; i < nCount; i++)
	{
		(*this)[i] = other[i];
	}
	return *this;
}

template<class T, class S>
inline void CQVector<T, S>::Construct(T* element)
{
	::new (element) T;
}

template<class T, class S>
inline void CQVector<T, S>::Destruct(T* element)
{
	m_Memory->~T();
}

template<typename T, class S>
inline T& CQVector<T, S>::operator[](int i)
{
	return m_Memory[i];
}

template<typename T, class S>
inline const T& CQVector<T, S>::operator[](int i) const
{
	return m_Memory[i];
}

template<class T, class S>
inline T& CQVector<T, S>::Element(int i)
{
	return m_Memory[i];
}

template<class T, class S>
inline const T& CQVector<T, S>::Element(int i) const
{
	return m_Memory[i];
}

template<class T, class S>
inline void CQVector<T, S>::GrowVector(int num)
{
	vecSize += num;
	RefreshElements();
}

template<class T, class S>
inline int CQVector<T, S>::Count() const
{
	return vecSize;
}

template<class T, class S>
inline void CQVector<T, S>::ShiftElementsRight(int elem, int num)
{
	int numToMove = vecSize - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
}

template<class T, class S>
inline void CQVector<T, S>::ShiftElementsLeft(int elem, int num)
{
	int numToMove = vecSize - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove(&Element(elem), &Element(elem + num), numToMove * sizeof(T));
}

template<class T, class S>
inline int CQVector<T, S>::AddToHead()
{
	return InsertBefore(0);
}

template<class T, class S>
inline int CQVector<T, S>::AddToTail()
{
	return InsertBefore(vecSize);
}

template<class T, class S>
inline int CQVector<T, S>::AddToHead(const T& src)
{
	return InsertBefore(0, src);
}

template<class T, class S>
inline int CQVector<T, S>::AddToTail(const T& src)
{
	return InsertBefore(vecSize, src);
}

template<class T, class S>
inline int CQVector<T, S>::AddMultipleToHead(int num)
{
	return InsertMultipleBefore(0, num);
}

template<class T, class S>
inline int CQVector<T, S>::AddMultipleToTail(int num, const T* pToCopy)
{
	return InsertMultipleBefore(vecSize, num, pToCopy);
}

template<class T, class S>
inline int CQVector<T, S>::InsertMultipleBefore(int elem, int num, const T* pToCopy)
{
	if (num == 0)
		return elem;

	GrowVector(num);
	ShiftElementsRight(elem, num);

	// Invoke default constructors
	for (int i = 0; i < num; ++i)
		Construct(&Element(elem + i));

	// Copy stuff in?
	if (pToCopy)
	{
		for (int i = 0; i < num; i++)
		{
			Element(elem + i) = pToCopy[i];
		}
	}

	return elem;
}

template<class T, class S>
inline int CQVector<T, S>::InsertMultipleAfter(int elem, int num)
{
	return InsertMultipleBefore(elem + 1, num);
}

template<class T, class S>
inline void CQVector<T, S>::RemoveAll()
{
	for (int i = vecSize; --i >= 0; )
	{
		delete &Element(i);
	}

	vecSize = 0;
}

template<class T, class S>
inline int CQVector<T, S>::InsertAfter(int elem)
{
	return InsertBefore(elem + 1);
}

template<class T, class S>
inline int CQVector<T, S>::InsertBefore(int elem)
{
	GrowVector();
	ShiftElementsRight(elem + 1);
	return elem;
}

template< typename T, class A >
void CQVector<T, A>::SetCount(int count)
{
	RemoveAll();
	AddMultipleToTail(count);
}

/* use our own copies of strlcpy and strlcat taken from OpenBSD */
extern size_t q_strlcpy(char* dst, const char* src, size_t size);

#endif