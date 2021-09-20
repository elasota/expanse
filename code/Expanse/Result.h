#pragma once

#include "BuildConfig.h"
#include "ErrorCode.h"
#include "ExpAssert.h"
#include "PreprocessorUtils.h"

namespace expanse
{
	struct Result
	{
	public:
		Result();
		Result(ErrorCode errorCode);
		Result(Result &&other);
		~Result();

		ErrorCode GetErrorCode() const;
		bool IsOK() const;
		void Handle();

		Result &operator=(Result &&other);

	private:
		Result(const Result &other) = delete;

		ErrorCode m_errorCode;
#if EXPANSE_DEBUG
		bool m_isHandled;
#endif
	};
}

namespace expanse
{
	inline Result::Result()
		: m_errorCode(ErrorCode::kOK)
#if EXPANSE_DEBUG
		, m_isHandled(true)
#endif
	{
	}

	inline Result::Result(ErrorCode errorCode)
		: m_errorCode(errorCode)
#if EXPANSE_DEBUG
		, m_isHandled(false)
#endif
	{
		EXP_ASSERT_RESULT(errorCode);
	}

	inline Result::Result(Result &&other)
		: m_errorCode(other.m_errorCode)
#if EXPANSE_DEBUG
		, m_isHandled(other.m_isHandled)
#endif
	{
		other.m_errorCode = ErrorCode::kOK;
#if EXPANSE_DEBUG
		other.m_isHandled = true;
#endif
	}

	inline Result::~Result()
	{
#if EXPANSE_DEBUG
		EXP_ASSERT(m_isHandled);
#endif
	}

	inline ErrorCode Result::GetErrorCode() const
	{
		return m_errorCode;
	}

	inline bool Result::IsOK() const
	{
		return m_errorCode == ErrorCode::kOK;
	}


	inline void Result::Handle()
	{
#if EXPANSE_DEBUG
		m_isHandled = true;
#endif
	}

	inline Result &Result::operator=(Result &&other)
	{
#if EXPANSE_DEBUG
		EXP_ASSERT(m_isHandled);
#endif

		m_errorCode = other.m_errorCode;
		m_isHandled = other.m_isHandled;

		other.m_errorCode = ErrorCode::kOK;
		other.m_isHandled = true;

		return *this;
	}
}


#define CHECK_R_ID(expr, varName)	\
	do\
	{\
		::expanse::Result varName(expr);\
		const ::expanse::ErrorCode errorCode = varName.GetErrorCode();\
		varName.Handle();\
		if (errorCode != ::expanse::ErrorCode::kOK)\
			return errorCode;\
	} while(false)

#define CHECK(expr) CHECK_R_ID(expr, EXP_CONCAT(expanse_check_r_, __COUNTER__))
