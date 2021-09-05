#pragma once

namespace expanse
{
	template<class T>
	class SimpleComparer
	{
	public:
		static bool Equal(const T &a, const T &b);
		static bool NotEqual(const T &a, const T &b);
		static bool Less(const T &a, const T &b);
		static bool Greater(const T &a, const T &b);
		static bool LessOrEqual(const T &a, const T &b);
		static bool GreaterOrEqual(const T &a, const T &b);
	};

	template<class T, bool TIsFloatingPoint>
	class DefaultComparer
	{
	};

	template<class T>
	class DefaultComparer<T, false> : public SimpleComparer<T>
	{
		static bool StrictlyEqual(const T &a, const T &b);
	};

	template<class T>
	class DefaultComparer<T, true> : public SimpleComparer<T>
	{
		static bool StrictlyEqual(const T &a, const T &b);
	};

	template<class T>
	class Comparer final : public DefaultComparer<T, std::is_floating_point<T>::value>
	{
	};
}

#include <string.h>

namespace expanse
{
	template<class T>
	bool DefaultComparer<T, true>::StrictlyEqual(const T &a, const T &b)
	{
		return !memcmp(a, b, sizeof(T));
	}

	template<class T>
	bool DefaultComparer<T, false>::StrictlyEqual(const T &a, const T &b)
	{
		return a == b;
	}

	template<class T>
	bool SimpleComparer<T>::Equal(const T &a, const T &b)
	{
		return a == b;
	}

	template<class T>
	bool SimpleComparer<T>::NotEqual(const T &a, const T &b)
	{
		return a != b;
	}

	template<class T>
	bool SimpleComparer<T>::Less(const T &a, const T &b)
	{
		return a < b;
	}

	template<class T>
	bool SimpleComparer<T>::Greater(const T &a, const T &b)
	{
		return a > b;
	}

	template<class T>
	bool SimpleComparer<T>::LessOrEqual(const T &a, const T &b)
	{
		return a <= b;
	}

	template<class T>
	bool SimpleComparer<T>::GreaterOrEqual(const T &a, const T &b)
	{
		return a >= b;
	}
}
