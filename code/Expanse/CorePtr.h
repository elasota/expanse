#pragma once

#include <cstddef>

namespace expanse
{
	class CoreObject;
	class ObjectAllocator;
	struct IAllocator;

	struct CorePtrBase
	{
	public:
		CorePtrBase();

	protected:
		explicit CorePtrBase(CoreObject *object);
		CorePtrBase(CorePtrBase &&other);
		~CorePtrBase();

		void TakeOwnershipFrom(CorePtrBase &other);

	private:
		CorePtrBase(const CorePtrBase &other) = delete;

	protected:
		friend class ObjectAllocator;

		CoreObject *m_object;
	};

	template<class T>
	struct CorePtr : public CorePtrBase
	{
	public:
		CorePtr();
		CorePtr(std::nullptr_t);
		template<class TOther>
		CorePtr(CorePtr<TOther> &&other);

		operator T *() const;
		T *operator->() const;
		T *Get() const;

		template<class TOther>
		CorePtr<T> &operator=(CorePtr<TOther> &&other);
		CorePtr<T> &operator=(std::nullptr_t);

		bool operator==(const T *other) const;
		bool operator!=(const T *other) const;
		bool operator<(const T *other) const;
		bool operator<=(const T *other) const;
		bool operator>(const T *other) const;
		bool operator>=(const T *other) const;

	private:
		friend class ObjectAllocator;

		explicit CorePtr(T *object);
		CorePtr(const CorePtr &other) = delete;
	};
}

#include "ExpAssert.h"
#include "Mem.h"

namespace expanse
{
	inline CorePtrBase::CorePtrBase()
		: m_object(nullptr)
	{
	}

	inline CorePtrBase::CorePtrBase(CoreObject *object)
		: m_object(object)
	{
	}

	inline CorePtrBase::CorePtrBase(CorePtrBase &&other)
		: m_object(other.m_object)
	{
		other.m_object = nullptr;
	}

	inline void CorePtrBase::TakeOwnershipFrom(CorePtrBase &other)
	{
		if (m_object != nullptr)
		{
			EXP_ASSERT(m_object != other.m_object);
			ObjectAllocator::DeleteObject(m_object);
		}

		m_object = other.m_object;
		other.m_object = nullptr;
	}

	inline CorePtrBase::~CorePtrBase()
	{
		if (m_object)
			ObjectAllocator::DeleteObject(m_object);
	}

	template<class T>
	CorePtr<T>::CorePtr()
	{
	}

	template<class T>
	CorePtr<T>::CorePtr(std::nullptr_t)
	{
	}

	template<class T>
	template<class TOther>
	CorePtr<T>::CorePtr(CorePtr<TOther> &&other)
	{
		this->TakeOwnershipFrom(other);
	}

	template<class T>
	CorePtr<T>::CorePtr(T *object)
		: CorePtrBase(object)
	{
	}

	template<class T>
	CorePtr<T>::operator T *() const
	{
		return static_cast<T*>(m_object);
	}

	template<class T>
	T *CorePtr<T>::operator->() const
	{
		return static_cast<T*>(m_object);
	}

	template<class T>
	T *CorePtr<T>::Get() const
	{
		return static_cast<T*>(m_object);
	}


	template<class T>
	template<class TOther>
	CorePtr<T> &CorePtr<T>::operator=(CorePtr<TOther> &&other)
	{
		if (static_cast<CorePtrBase*>(this) != static_cast<CorePtrBase*>(&other))
		{
			T *thisObj = static_cast<TOther*>(other);
			this->TakeOwnershipFrom(other);
		}
		return *this;
	}

	template<class T>
	CorePtr<T> &CorePtr<T>::operator=(std::nullptr_t)
	{
		if (m_object)
		{
			ObjectAllocator::DeleteObject(m_object);
			m_object = nullptr;
		}
		return *this;
	}


	template<class T>
	bool CorePtr<T>::operator==(const T *other) const
	{
		return static_cast<const T*>(m_object) == other;
	}

	template<class T>
	bool CorePtr<T>::operator!=(const T *other) const
	{
		return static_cast<const T*>(m_object) != other;
	}

	template<class T>
	bool CorePtr<T>::operator<(const T *other) const
	{
		return static_cast<const T*>(m_object) < other;
	}

	template<class T>
	bool CorePtr<T>::operator<=(const T *other) const
	{
		return static_cast<const T*>(m_object) <= other;
	}

	template<class T>
	bool CorePtr<T>::operator>(const T *other) const
	{
		return static_cast<const T*>(m_object) > other;
	}

	template<class T>
	bool CorePtr<T>::operator>=(const T *other) const
	{
		return static_cast<const T*>(m_object) >= other;
	}
}
