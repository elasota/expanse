#pragma once

#include "Hash.h"

#include <cstddef>
#include <type_traits>

namespace expanse
{
	namespace HashUtil
	{
		Hash_t ComputePODHash(const void *data, size_t size);
	}

	template<class T>
	class PODHasher
	{
	public:
		static Hash_t Compute(const T &key);
	};

	template<class T>
	class UseDefaultHash
	{
	public:
		static const bool kValue = std::is_arithmetic<T>::value;
	};

	template<class T, bool TIsPOD>
	class DefaultHasher
	{
	};

	template<class T>
	class DefaultHasher<T, true> : public PODHasher<T>
	{
	};

	template<class T>
	class Hasher final : public DefaultHasher<T, UseDefaultHash<T>::kValue>
	{
	public:
	};
}

namespace expanse
{
	template<class T>
	inline Hash_t PODHasher<T>::Compute(const T &key)
	{
		return HashUtil::ComputePODHash(&key, sizeof(key));
	}
}
