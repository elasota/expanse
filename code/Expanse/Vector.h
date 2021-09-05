#pragma once

#include "ArrayPtr.h"

namespace expanse
{
	template<class T> struct ArrayView;
	struct Result;

	template<class T>
	struct Vector
	{
	public:
		Vector();
		Vector(Vector<T> &&other);
		~Vector();

		size_t Size() const;
		size_t Capacity() const;
		operator ArrayView<T>() const;
		T &operator[](size_t index);
		const T &operator[](size_t index) const;

		Result Resize(size_t newSize);
		Result Add(T &&item);
		Result Add(const T &item);

	private:
		Vector(const Vector<T> &other) = delete;
		Result ReserveAdditional(size_t size);

		T *m_array;
		size_t m_size;
		size_t m_capacity;
	};
}

#include "ArrayView.h"
#include "Mem.h"

#include <new>

namespace expanse
{
	template<class T>
	Vector<T>::Vector()
		: m_array(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
	}

	template<class T>
	Vector<T>::Vector(Vector<T> &&other)
		: m_array(other.m_array)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
	{
		other.m_array = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
	}

	template<class T>
	Vector<T>::~Vector()
	{
		T *elements = m_array;
		const size_t size = 0;
		for (size_t i = 0; i < size; i++)
			elements[i].~T();
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
	Vector<T>::operator ArrayView<T>() const
	{
		return ArrayView<T>(m_array.GetBuffer(), m_size);
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

			Mem::Release(m_array);

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
			T *newArrayElements = static_cast<T*>(Mem::Alloc(sizeof(T) * newSize, alignof(T)));

			if (!newElements)
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
				Mem::Release(oldElements);
		}

		m_size = m_capacity = newSize;
	}

	template<class T>
	Result Vector<T>::Add(T &&item)
	{
		CHECK(ReserveAdditional(1));

		new (m_array + m_size) T(std::move(item));
		m_size++;
	}

	template<class T>
	Result Vector<T>::Add(const T &item)
	{
		CHECK(ReserveAdditional(1));

		new (m_array + m_size) T(item);
		m_size++;
	}

	template<class T>
	Result Vector<T>::ReserveAdditional(size_t size)
	{
		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);

		if (m_capacity - m_size < size)
			return ErrorCode::kOK;

		size_t newCapacity = 8;
		if (newCapacity > maxElements)
		{
			// Some really huge type, yikes
			newCapacity = maxElements;
		}
		else
		{
			while (newCapacity < size)
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
		const size_t size = m_size;

		T *newElements = static_cast<T*>(Mem::Alloc(sizeof(T)));
		if (!newElements)
			return ErrorCode::kOutOfMemory;

		for (size_t i = 0; i < size; i++)
		{
			new (newElements + i) T(std::move(oldElements[i]));
			oldElements[i].~T();
		}

		if (oldElements)
			Mem::Release(oldElements);

		m_array = newElements;
		m_capacity = newCapacity;

		return ErrorCode::kOK;
	}
}
