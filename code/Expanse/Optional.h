#pragma once


namespace expanse
{
	template<class T>
	class Optional
	{
	public:
		Optional();
		Optional(const Optional<T> &other);
		Optional(Optional<T> &&other);
		Optional(const T &other);
		~Optional();

		Optional<T> &operator=(const T &object);
		Optional<T> &operator=(T &&object);

		Optional<T> &operator=(const Optional<T> &object);
		Optional<T> &operator=(Optional<T> &&object);

		bool IsSet() const;
		void Clear();

		T &Modify();
		const T &Get() const;

	private:
		struct Nothing {};

		union ObjectOrNothingUnion
		{
			Nothing m_nothing;
			T m_object;

			ObjectOrNothingUnion();
			ObjectOrNothingUnion(const T &obj);
			ObjectOrNothingUnion(T &&obj);
			~ObjectOrNothingUnion();

			ObjectOrNothingUnion &operator=(const T &obj);
			ObjectOrNothingUnion &operator=(T &&obj);
		};

		ObjectOrNothingUnion m_u;
		bool m_isSet;
	};
}

#include <new>

#include "ExpAssert.h"

namespace expanse
{
	template<class T>
	Optional<T>::Optional()
		: m_isSet(false)
		, m_u()
	{
	}

	template<class T>
	Optional<T>::Optional(const Optional<T> &other)
		: m_isSet(other.m_isSet)
		, m_u()
	{
		if (other.m_isSet)
		{
			m_u.~ObjectOrNothingUnion();
			new (&m_u) ObjectOrNothingUnion(other.m_u.m_object);
		}
	}

	template<class T>
	Optional<T>::Optional(Optional<T> &&other)
		: m_isSet(other.m_isSet)
		, m_u()
	{
		if (other.m_isSet)
		{
			m_u.~ObjectOrNothingUnion();
			new (&m_u) ObjectOrNothingUnion(static_cast<T&&>(other.m_u.m_object));
		}
	}

	template<class T>
	Optional<T>::Optional(const T &other)
		: m_isSet(true)
		, m_u(other)
	{
	}

	template<class T>
	Optional<T>::~Optional()
	{
		if (m_isSet)
			m_u.m_object.~T();
		else
			m_u.m_nothing.~Nothing();

		m_u.~ObjectOrNothingUnion();
	}

	template<class T>
	Optional<T> &Optional<T>::operator=(const T &object)
	{
		if (m_isSet)
			m_u.m_object = object;
		else
		{
			m_u.m_nothing.~Nothing();
			m_u.~ObjectOrNothingUnion();
			new (&m_u) ObjectOrNothingUnion(object);
		}
		m_isSet = true;

		return *this;
	}

	template<class T>
	Optional<T> &Optional<T>::operator=(T &&object)
	{
		if (m_isSet)
			m_u.m_object = static_cast<T&&>(object);
		else
		{
			m_u.~ObjectOrNothingUnion();
			new (&m_u) ObjectOrNothingUnion(static_cast<T&&>(object));
		}
		m_isSet = true;

		return *this;
	}

	template<class T>
	Optional<T> &Optional<T>::operator=(const Optional<T> &other)
	{
		if (m_isSet)
		{
			if (other.m_isSet)
				m_u.m_object = other.m_u.m_object;
			else
			{
				m_u.m_object.~T();
				m_u.~ObjectOrNothingUnion();
				new (&m_u) ObjectOrNothingUnion();
				m_isSet = false;
			}
		}
		else
		{
			if (other.m_isSet)
			{
				m_u.~ObjectOrNothingUnion();
				new (&m_u) ObjectOrNothingUnion(other.m_u.m_object);
				m_isSet = true;
			}
		}

		return *this;
	}

	template<class T>
	Optional<T> &Optional<T>::operator=(Optional<T> &&other)
	{
		if (m_isSet)
		{
			if (other.m_isSet)
				m_u.m_object = std::move(other.m_u.m_object);
			else
			{
				m_u.m_object.~T();
				m_u.~ObjectOrNothingUnion();
				new (&m_u) ObjectOrNothingUnion();
				m_isSet = false;
			}
		}
		else
		{
			if (other.m_isSet)
			{
				m_u.~ObjectOrNothingUnion();
				new (&m_u) ObjectOrNothingUnion(std::move(other.m_u.m_object));
				m_isSet = true;
			}
		}

		return *this;
	}


	template<class T>
	bool Optional<T>::IsSet() const
	{
		return m_isSet;
	}

	template<class T>
	void Optional<T>::Clear()
	{
		if (m_isSet)
		{
			m_u.m_object.~T();
			m_u.~ObjectOrNothingUnion();
			new (&m_u) ObjectOrNothingUnion(Nothing());
			m_isSet = false;
		}
	}

	template<class T>
	T &Optional<T>::Modify()
	{
		EXP_ASSERT(m_isSet);
		return m_u.m_object;
	}

	template<class T>
	const T &Optional<T>::Get() const
	{
		EXP_ASSERT(m_isSet);
		return m_u.m_object;
	}

	template<class T>
	Optional<T>::ObjectOrNothingUnion::ObjectOrNothingUnion()
		: m_nothing()
	{
	}

	template<class T>
	Optional<T>::ObjectOrNothingUnion::ObjectOrNothingUnion(const T &obj)
		: m_object(obj)
	{
	}

	template<class T>
	Optional<T>::ObjectOrNothingUnion::ObjectOrNothingUnion(T &&obj)
		: m_object(static_cast<T&&>(obj))
	{
	}

	template<class T>
	Optional<T>::ObjectOrNothingUnion::~ObjectOrNothingUnion()
	{
	}

	template<class T>
	typename Optional<T>::ObjectOrNothingUnion &Optional<T>::ObjectOrNothingUnion::operator=(const T &obj)
	{
		m_object = obj;
		return *this;
	}

	template<class T>
	typename Optional<T>::ObjectOrNothingUnion &Optional<T>::ObjectOrNothingUnion::operator=(T &&obj)
	{
		m_object = static_cast<T&&>(obj);
		return *this;
	}
}
