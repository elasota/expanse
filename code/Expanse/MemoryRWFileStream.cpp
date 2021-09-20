#include "MemoryRWFileStream.h"

#include <algorithm>


namespace expanse
{
	MemoryRWFileStream::MemoryRWFileStream(IAllocator *alloc)
		: m_contents(alloc)
		, m_position(0)
	{
	}

	Result MemoryRWFileStream::SeekStart(UFilePos_t pos)
	{
		if (pos > m_contents.Size())
			return ErrorCode::kIOError;

		m_position = static_cast<size_t>(pos);

		return ErrorCode::kOK;
	}

	Result MemoryRWFileStream::SeekCurrent(FilePos_t pos)
	{
		const FilePos_t maxPos = std::numeric_limits<FilePos_t>::max();
		const size_t maxSize = std::numeric_limits<size_t>::max();

		const uintmax_t largestDelta = std::min<uintmax_t>(maxPos, maxSize);

		size_t targetPos = m_position;
		if (pos < 0)
		{
			const FilePos_t maxNegativePos = -maxPos;
			while (pos < maxNegativePos)
			{
				if (targetPos < largestDelta)
					return ErrorCode::kIOError;
				pos += static_cast<FilePos_t>(largestDelta);
				targetPos -= static_cast<size_t>(largestDelta);
			}

			const FilePos_t negativePos = -pos;
			if (negativePos > targetPos)
				return ErrorCode::kIOError;

			targetPos -= static_cast<size_t>(negativePos);
		}
		else
		{
			const size_t posMax = m_contents.Size() - m_position;
			if (static_cast<UFilePos_t>(pos) > posMax)
				return ErrorCode::kIOError;

			targetPos += static_cast<size_t>(pos);
		}

		m_position = targetPos;

		return ErrorCode::kOK;
	}

	Result MemoryRWFileStream::SeekEnd(FilePos_t pos)
	{
		const FilePos_t maxPos = std::numeric_limits<FilePos_t>::max();
		const size_t maxSize = std::numeric_limits<size_t>::max();

		const uintmax_t largestDelta = std::min<uintmax_t>(maxPos, maxSize);

		size_t targetPos = m_contents.Size();
		if (pos < 0)
		{
			const FilePos_t maxNegativePos = -maxPos;
			while (pos < maxNegativePos)
			{
				if (targetPos < largestDelta)
					return ErrorCode::kIOError;
				pos += static_cast<FilePos_t>(largestDelta);
				targetPos -= static_cast<size_t>(largestDelta);
			}

			const FilePos_t negativePos = -pos;
			if (negativePos > targetPos)
				return ErrorCode::kIOError;

			targetPos -= static_cast<size_t>(negativePos);
		}
		else if (pos == 0)
			return ErrorCode::kOK;
		else
			return ErrorCode::kIOError;

		m_position = targetPos;

		return ErrorCode::kOK;
	}

	ResultRV<UFilePos_t> MemoryRWFileStream::GetPosition() const
	{
		return m_position;
	}

	ResultRV<UFilePos_t> MemoryRWFileStream::GetSize() const
	{
		return m_contents.Size();
	}

	bool MemoryRWFileStream::IsReadable()
	{
		return true;
	}

	bool MemoryRWFileStream::IsWriteable()
	{
		return true;
	}

	ResultRV<ArrayPtr<uint8_t>> MemoryRWFileStream::ContentsToArray() const
	{
		const size_t contentsSize = m_contents.Size();
		if (contentsSize == 0)
			return ArrayPtr<uint8_t>();

		CHECK_RV(ArrayPtr<uint8_t>, arrayType, NewArrayUninitialized<uint8_t>(GetCoreObjectAllocator(), contentsSize));
		memcpy(&arrayType[0], &m_contents[0], contentsSize);

		return arrayType;
	}

	ResultRV<size_t> MemoryRWFileStream::Read(void *buffer, size_t size)
	{
		const size_t available = m_contents.Size() - m_position;
		const size_t amountToRead = std::min(available, size);

		if (amountToRead > 0)
			memcpy(buffer, &m_contents[m_position], amountToRead);

		m_position += amountToRead;

		return static_cast<size_t>(amountToRead);
	}

	ResultRV<size_t> MemoryRWFileStream::Write(const void *buffer, size_t size)
	{
		if (size == 0)
			return static_cast<size_t>(0);

		const size_t distanceFromEnd = m_contents.Size() - m_position;
		const size_t amountToCopy = std::min(distanceFromEnd, size);

		if (amountToCopy > 0)
		{
			memcpy(&m_contents[m_position], buffer, amountToCopy);
			buffer = static_cast<const void*>(static_cast<const uint8_t*>(buffer) + amountToCopy);
			size -= amountToCopy;
			m_position += amountToCopy;
		}

		CHECK(m_contents.Add(ArrayView<const uint8_t>(static_cast<const uint8_t*>(buffer), size)));
		m_position += size;

		return static_cast<size_t>(size + amountToCopy);
	}
}
