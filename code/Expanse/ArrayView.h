#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace expanse
{
	template<class T> struct ArrayPtr;
	template<class T> struct ResultRV;
	struct IAllocator;

	template<class T>
	struct ArrayView
	{
	public:
		ArrayView();
		template<size_t TSize>
		ArrayView(T (&elements)[TSize]);
		ArrayView(T *elements, size_t size);

		T &operator[](size_t index) const;
		size_t Size() const;

		operator ArrayView<const T>() const;

		ArrayView<T> Subrange(size_t startOffset, size_t length) const;
		ArrayView<T> Subrange(size_t startOffset) const;

		bool operator==(const ArrayView<T> &other) const;
		bool operator!=(const ArrayView<T> &other) const;

		ResultRV<ArrayPtr<typename std::remove_const<T>::type>> Clone(IAllocator *alloc) const;
		ResultRV<ArrayPtr<T>> CloneTake(IAllocator *alloc) const;

		T *begin() const;
		T *end() const;

	private:
		T *m_elements;
		size_t m_size;
	};
}

#include "ArrayPtr.h"
#include "ExpAssert.h"
#include "Mem.h"
#include "ResultRV.h"

#include <limits>

namespace expanse
{
	template<class T>
	ArrayView<T>::ArrayView()
		: m_elements(nullptr)
		, m_size(0)
	{
	}

	template<class T>
	template<size_t TSize>
	ArrayView<T>::ArrayView(T(&elements)[TSize])
		: m_elements(elements)
		, m_size(TSize)
	{
	}

	template<class T>
	ArrayView<T>::ArrayView(T *elements, size_t size)
		: m_elements(elements)
		, m_size(size)
	{
	}

	template<class T>
	T &ArrayView<T>::operator[](size_t index) const
	{
		EXP_ASSERT(index < m_size);
		return m_elements[index];
	}

	template<class T>
	size_t ArrayView<T>::Size() const
	{
		return m_size;
	}

	template<class T>
	ArrayView<T>::operator ArrayView<const T>() const
	{
		return ArrayView<const T>(m_elements, m_size);
	}

	template<class T>
	ArrayView<T> ArrayView<T>::Subrange(size_t startOffset, size_t length) const
	{
		EXP_ASSERT(startOffset <= m_size);
		EXP_ASSERT(m_size - startOffset >= length);

		return ArrayView<T>(m_elements + startOffset, length);
	}

	template<class T>
	ArrayView<T> ArrayView<T>::Subrange(size_t startOffset) const
	{
		EXP_ASSERT(startOffset <= m_size);

		return ArrayView<T>(m_elements + startOffset, m_size - startOffset);
	}

	template<class T>
	bool ArrayView<T>::operator==(const ArrayView<T> &other) const
	{
		return m_elements == other.m_elements && m_size == other.m_size;
	}

	template<class T>
	bool ArrayView<T>::operator!=(const ArrayView<T> &other) const
	{
		return !((*this) == other);
	}

	template<class T>
	ResultRV<ArrayPtr<typename std::remove_const<T>::type>> ArrayView<T>::Clone(IAllocator *alloc) const
	{
		typedef typename std::remove_const<T>::type NoConstT_t;

		const size_t count = this->m_size;
		if (count == 0)
			return ArrayPtr<NoConstT_t>();

		CHECK_RV(ArrayPtr<NoConstT_t>, clone, NewArrayUninitialized<NoConstT_t>(alloc, m_size));
		ArrayView<NoConstT_t> cloneView(clone);

		const T *thisElements = m_elements;
		NoConstT_t *cloneElements = &cloneView[0];

		for (size_t i = 0; i < count; i++)
			new (cloneElements + i) NoConstT_t(thisElements[i]);

		return clone;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> ArrayView<T>::CloneTake(IAllocator *alloc) const
	{
		const size_t count = this->m_size;
		if (count == 0)
			return ArrayPtr<T>();

		CHECK_RV(ArrayPtr<T>, clone, NewArrayUninitialized<T>(alloc, m_size));
		ArrayView<T> cloneView(clone);

		T *thisElements = m_elements;
		T *cloneElements = &cloneView[0];

		for (size_t i = 0; i < count; i++)
			new (cloneElements + i) T(std::move(thisElements[i]));

		return clone;
	}

	template<class T>
	T *ArrayView<T>::begin() const
	{
		return m_elements;
	}

	template<class T>
	T *ArrayView<T>::end() const
	{
		return m_elements + m_size;
	}
}
