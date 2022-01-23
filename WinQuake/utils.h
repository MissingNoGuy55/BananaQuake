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

	explicit CQVector(int startSize, int growSize = 1);
	explicit CQVector(T* memory, int startSize, int growSize = 1);
	~CQVector();

	CQVector<T, S>& operator=(const CQVector<T, S>& other);

	T* Base() { return m_pMemory.Base(); }
	const T* Base() const { return m_pMemory.Base(); }

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

	void ShiftElementsRight(int elem, int num);
	void ShiftElementsLeft(int elem, int num);

	int AddToHead();
	int AddToTail();

	int AddToHead(const T& src);
	int AddToTail(const T& src);

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
	}

//	const CCacheSystem* m_Cache;

	CMemAllocator m_pMemory;

	T* m_pElements; // variable-sized

	int vecSize;

private:

	CQVector(CQVector const& vec);

};

template<typename T, class S> CQVector<byte>::CQVector(byte);

template<class T, class S>
inline CQVector<T, S>::CQVector(int startSize, int growSize)
{
	m_pMemory = (T*)malloc(startSize * sizeof(T));
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

template<typename T, class S>
inline T& CQVector<T, S>::operator[](int i)
{
	return m_pMemory[i];
}

template<typename T, class S>
inline const T& CQVector<T, S>::operator[](int i) const
{
	return m_pMemory[i];
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
	size = vecSize - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
}

template<class T, class S>
inline void CQVector<T, S>::ShiftElementsLeft(int elem, int num)
{
	size = vecSize - elem - num;
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
inline int CQVector<T, S>::InsertAfter(int elem)
{
	return InsertBefore(elem + 1);
}

template<class T, class S>
inline int CQVector<T, S>::InsertBefore(int elem)
{
	GrowVector();
	ShiftElementsRight(elem + 1);
	return 0;
}


#endif