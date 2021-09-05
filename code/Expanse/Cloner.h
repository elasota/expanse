#pragma once

namespace expanse
{
	template<class T> struct ResultRV;

	template<class T>
	class Cloner
	{
	public:
		static ResultRV<T> Clone(const T &t);
	};
}

#include "ResultRV.h"

namespace expanse
{
	template<class T>
	ResultRV<T> Cloner<T>::Clone(const T &t)
	{
		return ResultRV<T>(T(t));
	}
}
