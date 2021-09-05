#pragma once

#include <cstddef>

namespace expanse
{
	template<class T> struct ArrayView;

	template<class T, size_t TSize>
	struct StaticArray
	{
	public:
		StaticArray();
		StaticArray(const StaticArray<T, TSize> &other);
		StaticArray(StaticArray<T, TSize> &&other);
		explicit StaticArray(const T &initializer);
		~StaticArray();

		StaticArray<T, TSize> &operator=(StaticArray<T, TSize> &&other) const;
		StaticArray<T, TSize> &operator=(const StaticArray<T, TSize> &other) const;

		operator ArrayView<T>();
		operator ArrayView<const T>() const;

		T &operator[](size_t index);
		const T &operator[](size_t index) const;

		static size_t Size();

	private:
		union DelayedInitElements
		{
			T m_elements[TSize];

			DelayedInitElements();
			~DelayedInitElements();
		};

		DelayedInitElements m_d;
	};
}

#include "ArrayView.h"
#include <new>

namespace expanse
{
	template<class T, size_t TSize>
	StaticArray<T, TSize>::StaticArray()
	{
		for (size_t i = 0; i < TSize; i++)
			new (m_d.m_elements + i) T();
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::StaticArray(const StaticArray<T, TSize> &other)
	{
		for (size_t i = 0; i < TSize; i++)
			new (m_d.m_elements + i) T(other.m_d.elements[i]);
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::StaticArray(StaticArray<T, TSize> &&other)
	{
		for (size_t i = 0; i < TSize; i++)
			new (m_d.m_elements + i) T(std::move(other.m_d.elements[i]));
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::StaticArray(const T &initializer)
	{
		for (size_t i = 0; i < TSize; i++)
			new (m_d.m_elements + i) T(initializer);
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::~StaticArray()
	{
		for (size_t i = 0; i < TSize; i++)
			m_d.m_elements[(TSize - 1) - i].~T();
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize> &StaticArray<T, TSize>::operator=(StaticArray<T, TSize> &&other) const
	{
		for (size_t i = 0; i < TSize; i++)
			m_d.m_elements[i] = std::move(other.m_d.m_elements[i]);

		return *this;
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize> &StaticArray<T, TSize>::operator=(const StaticArray<T, TSize> &other) const
	{
		for (size_t i = 0; i < TSize; i++)
			m_d.m_elements[i] = other.m_d.m_elements[i];

		return *this;
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::operator ArrayView<T>()
	{
		return ArrayView<T>(m_d.m_elements, TSize);
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::operator ArrayView<const T>() const
	{
		return ArrayView<const T>(m_d.m_elements, TSize);
	}

	template<class T, size_t TSize>
	T &StaticArray<T, TSize>::operator[](size_t index)
	{
		EXP_ASSERT(index <= TSize);
		return m_d.m_elements[index];
	}

	template<class T, size_t TSize>
	const T &StaticArray<T, TSize>::operator[](size_t index) const
	{
		EXP_ASSERT(index <= TSize);
		return m_d.m_elements[index];
	}

	static size_t Size();

	template<class T, size_t TSize>
	StaticArray<T, TSize>::DelayedInitElements::DelayedInitElements()
	{
	}

	template<class T, size_t TSize>
	StaticArray<T, TSize>::DelayedInitElements::~DelayedInitElements()
	{
	}
}
