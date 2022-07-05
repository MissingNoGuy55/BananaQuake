#pragma once

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
	CQVector<T, S>& operator=(CQVector<T, S>& other);

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
	void Init();

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

private:

	CMemAllocator m_Memory;

	T* m_pElements;

	int vecSize;

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
inline CQVector<T, S>::CQVector(CQVector const& vector) : m_Memory(memory, allocationCount), vecSize(numElements)
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
inline CQVector<T, S>& CQVector<T, S>::operator=(CQVector<T, S>& other)
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
inline void CQVector<T, S>::Init()
{
	m_pElements = NULL;
	vecSize = 0;

	m_Memory.Init();
	m_Memory.Grow(sizeof(T));
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

template< typename T, class S >
void CQVector<T, S>::SetCount(int count)
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

template<class T, size_t MAX_SIZE>
class CQArray
{
public:
	typedef T ElemType_t;
	typedef T* iterator;
	typedef const T* const_iterator;

	CQArray();
	CQArray(T* pMemory, size_t count);
	~CQArray();

	CQArray<T, MAX_SIZE>& operator=(const CQArray<T, MAX_SIZE>& other);
	CQArray(CQArray const& vec);

	// element access
	T& operator[](int i);
	const T& operator[](int i) const;
	T& Element(int i);
	const T& Element(int i) const;
	T& Random();
	const T& Random() const;

	T* Base();
	const T* Base() const;

	// Returns the number of elements in the array, NumAllocated() is included for consistency with UtlVector
	int Count() const;
	int NumAllocated() const;

	// Is element index valid?
	bool IsValidIndex(int i) const;
	static int InvalidIndex();

	void CopyArray(const T* pArray, size_t count);

	void Swap(CQArray< T, MAX_SIZE >& vec);

	// Finds an element (element needs operator== defined)
	int Find(const T& src) const;
	void FillWithValue(const T& src);

	bool HasElement(const T& src) const;

	// calls delete on each element in it.
	void DeleteElements();

protected:
	T m_Memory[MAX_SIZE];
};

template<class T, size_t MAX_SIZE>
inline CQArray<T, MAX_SIZE>::CQArray()
{
}

template<class T, size_t MAX_SIZE>
inline CQArray<T, MAX_SIZE>::CQArray(T* pMemory, size_t count)
{
}

template<class T, size_t MAX_SIZE>
inline CQArray<T, MAX_SIZE>::~CQArray()
{
}

template<class T, size_t MAX_SIZE>
inline CQArray<T, MAX_SIZE>& CQArray<T, MAX_SIZE>::operator=(const CQArray<T, MAX_SIZE>& other)
{
	if (this != &other)
	{
		for (size_t n = 0; n < MAX_SIZE; ++n)
		{
			m_Memory[n] = other.m_Memory[n];
		}
	}
	return *this;
}

template<class T, size_t MAX_SIZE>
inline CQArray<T, MAX_SIZE>::CQArray(CQArray const& vec)
{
}

template<class T, size_t MAX_SIZE>
inline T& CQArray<T, MAX_SIZE>::operator[](int i)
{
	return m_Memory[i];
}

template<class T, size_t MAX_SIZE>
inline const T& CQArray<T, MAX_SIZE>::operator[](int i) const
{
	return m_Memory[i];
}

template<class T, size_t MAX_SIZE>
inline T& CQArray<T, MAX_SIZE>::Element(int i)
{
	return m_Memory[i];
}

template<class T, size_t MAX_SIZE>
inline const T& CQArray<T, MAX_SIZE>::Element(int i) const
{
	return m_Memory[i];
}

template<class T, size_t MAX_SIZE>
inline T& CQArray<T, MAX_SIZE>::Random()
{
	// // O: insert return statement here
}

template<class T, size_t MAX_SIZE>
inline const T& CQArray<T, MAX_SIZE>::Random() const
{
	// // O: insert return statement here
}

template<class T, size_t MAX_SIZE>
inline T* CQArray<T, MAX_SIZE>::Base()
{
	return &m_Memory[0];
}

template<class T, size_t MAX_SIZE>
inline const T* CQArray<T, MAX_SIZE>::Base() const
{
	return &m_Memory[0];
}

template<class T, size_t MAX_SIZE>
inline int CQArray<T, MAX_SIZE>::Count() const
{
	return (int)MAX_SIZE;
}

template<class T, size_t MAX_SIZE>
inline int CQArray<T, MAX_SIZE>::NumAllocated() const
{
	return (int)MAX_SIZE;
}

template<class T, size_t MAX_SIZE>
inline bool CQArray<T, MAX_SIZE>::IsValidIndex(int i) const
{
	return (i >= 0) && (i < MAX_SIZE);
}

template<class T, size_t MAX_SIZE>
inline int CQArray<T, MAX_SIZE>::InvalidIndex()
{
	return -1;
}

template<class T, size_t MAX_SIZE>
inline void CQArray<T, MAX_SIZE>::CopyArray(const T* pArray, size_t count)
{
	for (size_t n = 0; n < count; ++n)
	{
		m_Memory[n] = pArray[n];
	}
}

template<class T, size_t MAX_SIZE>
inline void CQArray<T, MAX_SIZE>::Swap(CQArray<T, MAX_SIZE>& vec)
{
}

template<class T, size_t MAX_SIZE>
inline int CQArray<T, MAX_SIZE>::Find(const T& src) const
{
	for (int i = 0; i < Count(); ++i)
	{
		if (Element(i) == src)
			return i;
	}
	return -1;
}

template<class T, size_t MAX_SIZE>
inline void CQArray<T, MAX_SIZE>::FillWithValue(const T& src)
{
	for (int i = 0; i < Count(); i++)
	{
		Element(i) = src;
	}
}

template<class T, size_t MAX_SIZE>
inline bool CQArray<T, MAX_SIZE>::HasElement(const T& src) const
{
	return (Find(src) >= 0);
}

template<class T, size_t MAX_SIZE>
inline void CQArray<T, MAX_SIZE>::DeleteElements()
{
}
