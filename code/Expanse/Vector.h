#pragma once

#include "ArrayPtr.h"

namespace expanse
{
	template<class T> struct ArrayView;
	struct Result;
	struct IAllocator;

	template<class T>
	struct Vector
	{
	public:
		explicit Vector(IAllocator *alloc);
		Vector(Vector<T> &&other);
		~Vector();

		size_t Size() const;
		size_t Capacity() const;

		ArrayView<T> View();
		ArrayView<const T> ConstView() const;

		T &operator[](size_t index);
		const T &operator[](size_t index) const;

		Result Resize(size_t newSize);
		Result Add(T &&item);
		Result Add(const T &item);
		Result Add(const ArrayView<T> &elements);
		Result Add(const ArrayView<const T> &elements);

		Vector<T> &operator=(Vector<T> &&other);

	private:
		Vector(const Vector<T> &other) = delete;
		Result ReserveAdditional(size_t size);

		Vector<T> &operator=(const Vector<T> &other) = delete;

		T *m_array;
		IAllocator *m_alloc;
		size_t m_size;
		size_t m_capacity;
	};
}

#include "ArrayView.h"
#include "IAllocator.h"
#include "Result.h"

#include <new>

namespace expanse
{
	template<class T>
	Vector<T>::Vector(IAllocator *alloc)
		: m_array(nullptr)
		, m_size(0)
		, m_capacity(0)
		, m_alloc(alloc)
	{
	}

	template<class T>
	Vector<T>::Vector(Vector<T> &&other)
		: m_array(other.m_array)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
		, m_alloc(other.m_alloc)
	{
		other.m_array = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
	}

	template<class T>
	Vector<T>::~Vector()
	{
		T *elements = m_array;
		size_t size = m_size;
		while (size > 0)
		{
			size--;
			elements[size].~T();
		}
	}

	template<class T>
	inline size_t Vector<T>::Size() const
	{
		return m_size;
	}

	template<class T>
	inline size_t Vector<T>::Capacity() const
	{
		return m_capacity;
	}

	template<class T>
	ArrayView<T> Vector<T>::View()
	{
		return ArrayView<T>(m_array, m_size);
	}

	template<class T>
	ArrayView<const T> Vector<T>::ConstView() const
	{
		return ArrayView<T>(m_array, m_size);
	}

	template<class T>
	T &Vector<T>::operator[](size_t index)
	{
		EXP_ASSERT(index < m_size);
		return m_array[index];
	}

	template<class T>
	const T &Vector<T>::operator[](size_t index) const
	{
		EXP_ASSERT(index < m_size);
		return m_array[index];
	}

	template<class T>
	Result Vector<T>::Resize(size_t newSize)
	{
		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);

		if (newSize > maxElements)
			return ErrorCode::kOutOfMemory;

		if (newSize == 0)
		{
			T *elements = m_array;
			size_t oldSize = m_size;

			for (size_t i = 0; i < oldSize; i++)
				elements[oldSize - i - 1].~T();

			m_alloc->Release(elements);

			m_size = 0;
			m_capacity = 0;
			m_array = nullptr;
			return ErrorCode::kOK;
		}

		const size_t oldSize = m_size;

		if (m_capacity == newSize)
		{
			T *elements = m_array;

			if (newSize <= oldSize)
			{
				for (size_t i = oldSize; i < newSize; i++)
					new (elements + i) T();
			}
			else
			{
				for (size_t i = newSize; i < oldSize; i++)
					elements[i].~T();
			}
		}
		else
		{
			T *oldElements = m_array;
			T *newArrayElements = static_cast<T*>(m_alloc->Alloc(sizeof(T) * newSize, alignof(T)));

			if (!newArrayElements)
				return ErrorCode::kOutOfMemory;

			if (newSize <= oldSize)
			{
				for (size_t i = 0; i < newSize; i++)
					new (newArrayElements + i) T(std::move(oldElements[i]));
			}
			else
			{
				for (size_t i = 0; i < oldSize; i++)
					new (newArrayElements + i) T(std::move(oldElements[i]));
				for (size_t i = oldSize; i < newSize; i++)
					new (newArrayElements + i) T();
			}

			for (size_t i = 0; i < oldSize; i++)
				oldElements[i].~T();

			m_array = newArrayElements;

			if (oldElements)
				m_alloc->Release(oldElements);
		}

		m_size = m_capacity = newSize;

		return ErrorCode::kOK;
	}

	template<class T>
	Result Vector<T>::Add(T &&item)
	{
		CHECK(ReserveAdditional(1));

		new (m_array + m_size) T(std::move(item));
		m_size++;

		return ErrorCode::kOK;
	}

	template<class T>
	Result Vector<T>::Add(const T &item)
	{
		CHECK(ReserveAdditional(1));

		new (m_array + m_size) T(item);
		m_size++;

		return ErrorCode::kOK;
	}

	template<class T>
	Result Vector<T>::Add(const ArrayView<T> &elementsRef)
	{
		return Add(ArrayView<const T>(elementsRef));
	}

	template<class T>
	Result Vector<T>::Add(const ArrayView<const T> &elementsRef)
	{
		const ArrayView<const T> elements = elementsRef;

		const size_t numElementsToAdd = elements.Size();
		if (numElementsToAdd == 0)
			return ErrorCode::kOK;

		CHECK(ReserveAdditional(numElementsToAdd));

		T *insertionPoint = m_array + m_size;
		const T *elementsPtr = &elements[0];

		for (size_t i = 0; i < numElementsToAdd; i++)
			new (insertionPoint + i) T(elementsPtr[i]);

		m_size += numElementsToAdd;

		return ErrorCode::kOK;
	}

	template<class T>
	Vector<T> &Vector<T>::operator=(Vector<T> &&other)
	{
		if (this != &other)
		{
			m_array = other.m_array;
			m_size = other.m_size;
			m_capacity = other.m_capacity;
			m_alloc = other.m_alloc;

			other.m_array = nullptr;
			other.m_size = 0;
			other.m_capacity = 0;
			other.m_alloc = nullptr;
		}

		return *this;
	}

	template<class T>
	Result Vector<T>::ReserveAdditional(size_t numAdditional)
	{
		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		const size_t size = m_size;

		if (m_capacity - numAdditional < numAdditional)
			return ErrorCode::kOK;

		const size_t newCapacityRequired = size + numAdditional;
		size_t newCapacity = 8;
		if (newCapacity > maxElements)
		{
			// Some really huge type, yikes
			newCapacity = maxElements;
		}
		else
		{
			while (newCapacity < newCapacityRequired)
			{
				const size_t maxExpandable = maxElements - newCapacity;
				if (maxExpandable == 0)
					return ErrorCode::kOutOfMemory;

				size_t expansion = newCapacity / 2;
				if (expansion > maxExpandable)
					expansion = maxExpandable;

				newCapacity += expansion;
			}
		}

		T *oldElements = m_array;
		const size_t oldCapacity = m_capacity;

		T *newElements = static_cast<T*>(m_alloc->Alloc(sizeof(T) * newCapacity, alignof(T)));
		if (!newElements)
			return ErrorCode::kOutOfMemory;

		for (size_t i = 0; i < size; i++)
		{
			new (newElements + i) T(std::move(oldElements[i]));
			oldElements[i].~T();
		}

		if (oldElements)
			m_alloc->Release(oldElements);

		m_array = newElements;
		m_capacity = newCapacity;

		return ErrorCode::kOK;
	}
}
