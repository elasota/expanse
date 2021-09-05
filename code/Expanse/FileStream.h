#pragma once

#include "CoreObject.h"

#include <stdint.h>

namespace expanse
{
	template<class T> struct ArrayView;
	struct Result;
	template<class T> struct ResultRV;

	typedef uint64_t UFilePos_t;
	typedef int64_t FilePos_t;

	class FileStream : public CoreObject
	{
	public:
		virtual Result SeekStart(UFilePos_t pos) = 0;
		virtual Result SeekCurrent(FilePos_t pos) = 0;
		virtual Result SeekEnd(FilePos_t pos) = 0;

		virtual ResultRV<UFilePos_t> GetPosition() const = 0;
		virtual ResultRV<UFilePos_t> GetSize() const = 0;

		virtual bool IsReadable() = 0;
		virtual bool IsWriteable() = 0;

		template<class T>
		ResultRV<size_t> ReadPartial(const ArrayView<T> &arrayView);

	protected:
		virtual ResultRV<size_t> Read(void *buffer, size_t size) = 0;
	};
}

#include "ArrayView.h"
#include "ResultRV.h"

namespace expanse
{
	template<class T>
	ResultRV<size_t> FileStream::ReadPartial(const ArrayView<T> &arrayView)
	{
		if (arrayView.Size() == 0)
			return 0;

		CHECK_RV(size_t, numBytesRead, this->Read(&arrayView[0], sizeof(T) * arrayView.Size()));
		return numBytesRead / sizeof(T);
	}
}
