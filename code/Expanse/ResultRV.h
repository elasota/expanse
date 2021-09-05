#pragma once

#include "BuildConfig.h"
#include "ErrorCode.h"

#include <cstdint>

namespace expanse
{
	template<class T>
	struct ResultRV
	{
	public:
		ResultRV(const T &value);
		ResultRV(T &&value);
		ResultRV(ErrorCode errorCode);
		ResultRV(ResultRV<T> &&other);
		~ResultRV();

		T TakeValue();
		const T &GetValue();
		ErrorCode GetErrorCode() const;
		void Handle();

		ResultRV<T> &operator=(ResultRV<T> &&other);

	private:
		ResultRV(const ResultRV<T> &other) = delete;

		union OptionalResult
		{
			T m_result;

			explicit OptionalResult();
			OptionalResult(const T &result);
			OptionalResult(T &&result);
			~OptionalResult();
		};

		OptionalResult m_optionalResult;
		ErrorCode m_errorCode;
#if EXPANSE_DEBUG
		bool m_isHandled;
#endif
	};
}

#include "ExpAssert.h"

#include <utility>

namespace expanse
{
	template<class T>
	ResultRV<T>::ResultRV(const T &value)
		: m_optionalResult(value)
		, m_errorCode(ErrorCode::kOK)
#if EXPANSE_DEBUG
		, m_isHandled(false)
#endif
	{
	}

	template<class T>
	ResultRV<T>::ResultRV(T &&value)
		: m_optionalResult(std::move(value))
		, m_errorCode(ErrorCode::kOK)
#if EXPANSE_DEBUG
		, m_isHandled(false)
#endif
	{
	}

	template<class T>
	ResultRV<T>::ResultRV(ErrorCode errorCode)
		: m_optionalResult()
		, m_errorCode(errorCode)
#if EXPANSE_DEBUG
		, m_isHandled(false)
#endif
	{
		EXP_ASSERT(errorCode != ErrorCode::kOK);
	}

	template<class T>
	ResultRV<T>::ResultRV(ResultRV<T> &&other)
		: m_optionalResult()
		, m_errorCode(other.m_errorCode)
#if EXPANSE_DEBUG
		, m_isHandled(other.m_isHandled)
#endif
	{
		if (other.m_errorCode == ErrorCode::kOK)
		{
			m_optionalResult.~OptionalResult();
			new (&m_optionalResult) OptionalResult(std::move(other.m_optionalResult.m_result));

			other.m_optionalResult.m_result.~T();
		}

		other.m_errorCode = ErrorCode::kOK;

#if EXPANSE_DEBUG
		other.m_isHandled = true;
#endif
	}

	template<class T>
	ResultRV<T>::~ResultRV()
	{
#if EXPANSE_DEBUG
		EXP_ASSERT(m_isHandled);
#endif

		if (m_errorCode == ErrorCode::kOK)
			m_optionalResult.m_result.~T();
	}

	template<class T>
	T ResultRV<T>::TakeValue()
	{
		EXP_ASSERT(m_errorCode == ErrorCode::kOK);
		return T(std::move(m_optionalResult.m_result));
	}

	template<class T>
	const T &ResultRV<T>::GetValue()
	{
		EXP_ASSERT(m_errorCode == ErrorCode::kOK);
		return m_optionalResult.m_result;
	}

	template<class T>
	ErrorCode ResultRV<T>::GetErrorCode() const
	{
		return m_errorCode;
	}

	template<class T>
	void ResultRV<T>::Handle()
	{
#if EXPANSE_DEBUG
		m_isHandled = true;
#endif
	}

	template<class T>
	ResultRV<T> &ResultRV<T>::operator=(ResultRV<T> &&other)
	{
#if EXPANSE_DEBUG
		EXP_ASSERT(m_isHandled);
#endif

		if (this != &other)
		{
			if (m_errorCode == ErrorCode::kOK)
			{
				if (other.m_errorCode == ErrorCode::kOK)
					m_optionalResult.m_result = std::move(other.m_optionalResult.m_result);
				else
				{
					m_optionalResult.m_result.~OptionalResult();
					new (&m_optionalResult) OptionalResult();
				}
			}
			else
			{
				if (other.m_errorCode == ErrorCode::kOK)
				{
					m_optionalResult.m_result.~OptionalResult();
					new (&m_optionalResult) OptionalResult(std::move(other.m_optionalResult.m_result));
				}
			}

			m_errorCode = other.m_errorCode;
			other.m_errorCode = ErrorCode::kOK;

#if EXPANSE_DEBUG
			m_isHandled = other.m_isHandled;
			other.m_isHandled = true;
#endif
		}
	}

	template<class T>
	ResultRV<T>::OptionalResult::OptionalResult()
	{
	}

	template<class T>
	ResultRV<T>::OptionalResult::OptionalResult(const T &result)
		: m_result(result)
	{
	}

	template<class T>
	ResultRV<T>::OptionalResult::OptionalResult(T &&result)
		: m_result(std::move(result))
	{
	}

	template<class T>
	ResultRV<T>::OptionalResult::~OptionalResult()
	{
	}
}

#include "PreprocessorUtils.h"

#define CHECK_RV_ID(type, name, expr, varName)	\
	::expanse::ResultRV<type> varName(expr);\
	{\
		const ::expanse::ErrorCode errorCode = varName.GetErrorCode();\
		varName.Handle();\
		if (errorCode != ::expanse::ErrorCode::kOK)\
			return errorCode;\
	}\
	type name(varName.TakeValue())

#define CHECK_RV(type, name, expr) CHECK_RV_ID(type, name, expr, EXP_CONCAT(expanse_check_rrv_, __COUNTER__))
