#pragma once

#include <vector>

#ifndef UTILS_H
#define UTILS_H

class CCoreRenderer;
class CSoftwareRenderer;
class CGLRenderer;
class CGLTexture;

// Missi: C++ library stuff

#ifndef vector					

template<class T, class A = CMemBlock< T >>
class CQuakeVector
{
public:

	explicit CQuakeVector();
	explicit CQuakeVector(const T& src);

	T* vector;
	A mem;

	typedef T Vector;

	bool Empty() const;

	const T& Front() const;

	T& Front();

	const T& Back() const;

	T& Back();

	const T& At(size_t index) const;

	T& At(size_t index);

	void ClearAll();

	T* operator=(const T* src);
	const T* operator=(const T* right) const;

	T operator+(const int amt);

	T& operator[](size_t index) { return vector[index]; }
	const T& operator[](size_t index) const { return vector[index]; }

	bool operator==(const T& in);
	const bool operator==(const T& in) const;
	bool operator!=(const T& in);
	const bool operator!=(const T& in) const;

	void* operator new(size_t sz);

	T* Data();
	const T* Data() const;

	T& End();

	const T& End() const;

	/*void Push_Back(T* src);*/

	int AddToFront();
	int AddToBack();

	void Push_Back(T& src);

	void Reserve(size_t amount);

	void Reallocate(size_t amount);
	T* Allocate(size_t amount);

	size_t Count();
	size_t Size();


private:

	size_t size;
	size_t count;
	size_t max_size;

};

template<class T, class A>
inline CQuakeVector<T, A>::CQuakeVector()
{
	vector = NULL;/*(T*)calloc(1, sizeof(T));*/
	size = 0;
	count = 0;
	max_size = 0;
	mem = (A)NULL;
}

template<class T, class A>
inline CQuakeVector<T, A>::CQuakeVector(const T& src)
{
	vector = NULL;/*(T*)calloc(1, sizeof(T));*/
	size = 0;
	count = 0;
	max_size = 0;
	mem = (A)NULL;
}

template<class T, class A>
inline bool CQuakeVector<T, A>::Empty() const
{
	return (size > 0);
}

template<class T, class A>
inline const T& CQuakeVector<T, A>::Front() const
{
	return vector[count];
}

template<class T, class A>
inline T& CQuakeVector<T, A>::Front()
{
	return vector[count];
}

template<class T, class A>
inline const T& CQuakeVector<T, A>::Back() const
{
	return vector[0];// // O: insert return statement here
}

template<class T, class A>
inline T& CQuakeVector<T, A>::Back()
{
	return vector[0];// // O: insert return statement here
}

template<class T, class A>
inline T& CQuakeVector<T, A>::At(size_t index)
{
	return vector[index];
}

template<class T, class A>
inline void CQuakeVector<T, A>::ClearAll()
{
	for (int i = 0; i < count; i++)
	{
		vector[i] = NULL;
	}
}

template<class T, class A>
inline const T& CQuakeVector<T, A>::At(size_t index) const
{
	return vector[index];
}

template<class T, class A>
inline T* CQuakeVector<T, A>::operator=(const T* src)
{
	if (src)
	{
		return (T*)memcpy(vector, src, sizeof(T));
	}
	else
	{
		printf("FUCK!!!\n");
	}
}

template<class T, class A>
inline const T* CQuakeVector<T, A>::operator=(const T* src) const
{
	return this->vector;
}

template<class T, class A>
inline T CQuakeVector<T, A>::operator+(const int b)
{
	T bb;
	vector = vector + b;
	return (*vector);
}

template<class T, class A>
inline bool CQuakeVector<T, A>::operator==(const T& in)
{
	return false;
}

template<class T, class A>
inline const bool CQuakeVector<T, A>::operator==(const T& in) const
{
	return false;
}

template<class T, class A>
inline bool CQuakeVector<T, A>::operator!=(const T& in)
{
	return false;
}

template<class T, class A>
inline const bool CQuakeVector<T, A>::operator!=(const T& in) const
{
	return false;
}

template<class T, class A>
inline void* CQuakeVector<T, A>::operator new(size_t sz)
{
	return ::new T;
}

template<class T, class A>
inline T* CQuakeVector<T, A>::Data()
{
	if (Empty())
		return nullptr;

	return vector;
}

template<class T, class A>
inline const T* CQuakeVector<T, A>::Data() const
{
	if (Empty())
		return nullptr;

	return vector;
}

template<class T, class A>
inline T& CQuakeVector<T, A>::End()
{
	return vector[count];
}

template<class T, class A>
inline const T& CQuakeVector<T, A>::End() const
{
	return vector[count];
}

template<class T, class A>
inline void CQuakeVector<T, A>::Push_Back(T& src)
{
	vector[size+1] = src;
}

template<class T, class A>
inline void CQuakeVector<T, A>::Reserve(size_t amount)
{
	if (size < count)
	{	// something to do, check and reallocate
		if (max_size < count)
			count = Count();
		Reallocate(amount);
	}
}

template<class T, class A>
inline void CQuakeVector<T, A>::Reallocate(size_t amount)
{
	T* test = this->Allocate(amount);
}

#define _BIG_ALLOCATION_THRESHOLD	4096
#define _BIG_ALLOCATION_ALIGNMENT	32

#define _NON_USER_SIZE (sizeof(void *) + _BIG_ALLOCATION_ALIGNMENT - 1)

template<class T, class A>
inline T* CQuakeVector<T, A>::Allocate(size_t amount)
{

	T* Ptr = 0;

	Ptr = (T*)calloc(amount, sizeof(T));

		if (!Ptr)
			return NULL;

		vector = Ptr;
		size += amount;
		count += amount;

		return Ptr;

}

template<class T, class A>
inline size_t CQuakeVector<T, A>::Count()
{
		size_t _Count = 0;
	
		for (size_t _Index = 0; _Index < Size(); _Index++)
		{
			if (&this[_Index] != nullptr)
			{
				_Count++;
			}
		}
	
		return _Count;
}

template<class T, class A>
inline size_t CQuakeVector<T, A>::Size()
{
	return size;
}

#endif


// Missi: CQVector -- copied CUtlVector from Source. (3/8/2022)
// !!!
// !!!WARNING!!! Elements are moved around in memory every time a new element is added. Never maintain pointers to any element of a CUtlVector object.
// !!!
// Instead, pass the element as an argument.
// e.g. vector->Element(index)

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

//	const CMemCacheSystem* m_Cache;

	CMemAllocator m_Memory;

	T* m_pElements;

	int vecSize;

private:

	CQVector(CQVector const& vector);

};

// template<class T, class S> CQVector<byte>::CQVector(byte);

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
	new (element) T;
}

template<class T, class S>
void CQVector<T, S>::CopyConstruct(T* element, const T& src)
{
	new (element) T(src);
}

template<class T, class S>
void CQVector<T, S>::Destruct(T* element)
{
	element->~T();
}

template<class T, class S>
inline T& CQVector<T, S>::operator[](int i)
{
//#ifdef _DEBUG
//	if ((unsigned)i > (unsigned)vecSize);
//		return m_Memory[0];
//#endif
	return m_Memory[i];
}

template<class T, class S>
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
		for (int i = 0; i < amount; i++)
		{
			(*this)[i] = (T)AddToTail(src);
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