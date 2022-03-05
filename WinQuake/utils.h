#pragma once

#ifndef UTILS_H
#define UTILS_H

class CCacheSystem;

// CQVector -- copied CUtlVector from Source.

template<class T, class S = CMemBlock<T> >
class CQVector
{
	typedef S CMemAllocator;
public:

	typedef T ElementType;
	
	explicit CQVector(int startSize = 0, int growSize = 0);
	explicit CQVector(T* memory, int allocationCount, int numElements = 0);
	~CQVector();

	CQVector<T, S>& operator=(const CQVector<T, S>& other);

	void Construct(T* element);
	void CopyConstruct(T* element, const T& src);
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

	void ShiftElementsRight(int elem, int num = 1);
	void ShiftElementsLeft(int elem, int num = 1);

	void PrintContents();

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

	bool IsValidElement(int i);

	// Returns the number of elements in the vector
	int Count() const;
	void SetCount(int count);

	void Purge();
	void GrowVector(int num = 1);
	void Fill(const T& src = NULL, int amount = 0);
	void Compact();

	// Is the vector empty?
	inline bool IsEmpty(void) const
	{
		return (Count() == 0);
	}

	inline void RefreshElements()
	{
		m_pElements = Base();
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
CQVector<T, S>::CQVector(int growSize, int startSize) : m_Memory(growSize, startSize), vecSize(0)
{
	RefreshElements();
}

template<class T, class S>
CQVector<T, S>::CQVector(T* memory, int allocationCount, int numElements) : m_Memory(memory, allocationCount), vecSize(numElements)
{
	RefreshElements();
}

template<class T, class S>
CQVector<T, S>::~CQVector()
{
	Purge();
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
void CQVector<T, S>::Construct(T* element)
{
	::new (element) T;
}

template<class T, class S>
void CQVector<T, S>::CopyConstruct(T* element, const T& src)
{
	::new (element) T(src);
}

template<class T, class S>
void CQVector<T, S>::Destruct(T* element)
{
	element->~T();
}

template<typename T, class S>
inline T& CQVector<T, S>::operator[](int i)
{
//#ifdef _DEBUG
//	if ((unsigned)i > (unsigned)vecSize);
//		return m_Memory[0];
//#endif
	return m_Memory[i];
}

template<typename T, class S>
inline const T& CQVector<T, S>::operator[](int i) const
{
//#ifdef _DEBUG
//	if ((unsigned)i > (unsigned)vecSize);
//		return m_Memory[0];
//#endif
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
inline T& CQVector<T, S>::Head()
{
	return m_Memory[0];
}

template<class T, class S>
inline const T& CQVector<T, S>::Head() const
{
	return m_Memory[0];
}

template<class T, class S>
inline T& CQVector<T, S>::Tail()
{
	return m_Memory[vecSize - 1];
}

template<class T, class S>
inline const T& CQVector<T, S>::Tail() const
{
	return m_Memory[vecSize - 1];
}

template<class T, class S>
inline void CQVector<T, S>::Purge()
{
	RemoveAll();
	m_Memory.Purge();
	RefreshElements();
}

template<class T, class S>
void CQVector<T, S>::GrowVector(int num)
{

	if (vecSize + num > m_Memory.NumAllocated())
	{
		m_Memory.Grow(vecSize + num - m_Memory.NumAllocated());
	}

	vecSize += num;
	RefreshElements();
}

template<class T, class S>
void CQVector<T, S>::Fill(const T& src, int amount)
{
	if (amount == 0)
	{
		return;
	}

	if (&src)
	{
		for (int i = 0; i < Count(); i++)
		{
			(*this)[i] = (&src)[i];
		}
		RefreshElements();
	}
}

template<class T, class S>
inline int CQVector<T, S>::Count() const
{
	return vecSize;
}

template<class T, class S>
void CQVector<T, S>::ShiftElementsRight(int elem, int num)
{
	int numToMove = vecSize - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
}

template<class T, class S>
void CQVector<T, S>::ShiftElementsLeft(int elem, int num)
{
	int numToMove = vecSize - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove(&Element(elem), &Element(elem + num), numToMove * sizeof(T));
}

template<class T, class S>
inline void CQVector<T, S>::PrintContents()
{
	for (int i = 0; i < Count(); i++)
	{
		Con_Printf("Element %d is %c\n", i, Element(i));
	}
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
	/*if (Base() == NULL || (&src < Base()) || (&src >= (Base() + Count())))
		return -1;*/

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
void CQVector<T, S>::RemoveAll()
{
	for (int i = vecSize; --i >= 0; )
	{
		Destruct(&Element(i));
	}

	vecSize = 0;
}

template<class T, class S>
int CQVector<T, S>::InsertAfter(int elem)
{
	return InsertBefore(elem + 1);
}

template<class T, class S>
int CQVector<T, S>::InsertBefore(int elem)
{

	/*if (elem > Count() || !IsValidElement(elem))
		return -1;*/

	GrowVector();
	ShiftElementsRight(elem);
	Construct(&Element(elem));
	return elem;
}

template<class T, class S>
int CQVector<T, S>::InsertBefore(int elem, const T& src)
{
	/*if (Base() != NULL || (&src > Base()) || (&src < (Base() + Count())))
		return -1;*/

	GrowVector();
	ShiftElementsRight(elem);
	CopyConstruct(&Element(elem), src);
	return elem;
}

template<class T, class S>
inline int CQVector<T, S>::InsertAfter(int elem, const T& src)
{
	return InsertBefore(elem + 1, src);
}

template<class T, class S>
inline bool CQVector<T, S>::IsValidElement(int i)
{
	return (i >= 0) && (i < vecSize);
}

template< typename T, class A >
void CQVector<T, A>::SetCount(int count)
{
	RemoveAll();
	AddMultipleToTail(count);
}

template<class T, class S>
inline void CQVector<T, S>::Compact()
{
	m_Memory.Purge(vecSize);
	RefreshElements();
}

/* use our own copies of strlcpy and strlcat taken from OpenBSD */
extern size_t q_strlcpy(char* dst, const char* src, size_t size);

#endif