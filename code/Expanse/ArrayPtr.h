#pragma once

#include <cstddef>

namespace expanse
{
	class CoreObject;
	class ObjectAllocator;
	struct IAllocator;
	template<class T> struct ArrayView;

	template<class T>
	struct ArrayPtr
	{
	public:
		ArrayPtr();
		ArrayPtr(std::nullptr_t);
		ArrayPtr(ArrayPtr<T> &&other);
		~ArrayPtr();

		T &operator[](size_t index) const;
		size_t Count() const;

		bool operator==(std::nullptr_t) const;
		bool operator!=(std::nullptr_t) const;

		operator ArrayView<T>() const;

		ArrayView<T> View() const;
		ArrayView<const T> ConstView() const;

		T *GetBuffer() const;

		ArrayPtr<T> &operator=(ArrayPtr<T> &&other);

	private:
		friend class ObjectAllocator;

		explicit ArrayPtr(IAllocator *alloc, T *elements, size_t size);
		ArrayPtr(const ArrayPtr<T> &other) = delete;

		T *m_elements;
		size_t m_size;

		IAllocator *m_alloc;
	};
}

#include "ExpAssert.h"
#include "ArrayView.h"
#include "Mem.h"

namespace expanse
{
	template<class T>
	inline ArrayPtr<T>::ArrayPtr()
		: m_elements(nullptr)
		, m_size(0)
		, m_alloc(nullptr)
	{
	}

	template<class T>
	inline ArrayPtr<T>::ArrayPtr(std::nullptr_t)
		: m_elements(nullptr)
		, m_size(0)
		, m_alloc(nullptr)
	{
	}

	template<class T>
	inline ArrayPtr<T>::ArrayPtr(ArrayPtr<T> &&other)
		: m_elements(other.m_elements)
		, m_size(other.m_size)
		, m_alloc(other.m_alloc)
	{
		other.m_elements = nullptr;
		other.m_size = 0;
		other.m_alloc = nullptr;
	}

	template<class T>
	inline ArrayPtr<T>::~ArrayPtr()
	{
		if (m_elements != nullptr)
			ObjectAllocator::DeleteArray<T>(*this);
	}

	template<class T>
	inline T &ArrayPtr<T>::operator[](size_t index) const
	{
		EXP_ASSERT(index < m_size);
		return m_elements[index];
	}

	template<class T>
	inline size_t ArrayPtr<T>::Count() const
	{
		return m_size;
	}

	template<class T>
	bool ArrayPtr<T>::operator==(std::nullptr_t) const
	{
		return m_elements == nullptr;
	}

	template<class T>
	bool ArrayPtr<T>::operator!=(std::nullptr_t) const
	{
		return m_elements != nullptr;
	}

	template<class T>
	ArrayView<T> ArrayPtr<T>::View() const
	{
		return ArrayView<T>(m_elements, m_size);
	}

	template<class T>
	ArrayView<const T> ArrayPtr<T>::ConstView() const
	{
		return ArrayView<const T>(m_elements, m_size);
	}

	template<class T>
	ArrayPtr<T>::operator ArrayView<T>() const
	{
		return this->View();
	}

	template<class T>
	T *ArrayPtr<T>::GetBuffer() const
	{
		return m_elements;
	}

	template<class T>
	ArrayPtr<T> &ArrayPtr<T>::operator=(ArrayPtr<T> &&other)
	{
		if (this != &other)
		{
			T *oldElements = m_elements;
			m_elements = other.m_elements;
			m_size = other.m_size;

			other.m_elements = nullptr;
			other.m_size = 0;
		}

		return *this;
	}

	template<class T>
	ArrayPtr<T>::ArrayPtr(IAllocator *alloc, T *elements, size_t size)
		: m_elements(elements)
		, m_size(size)
		, m_alloc(alloc)
	{
	}
}
