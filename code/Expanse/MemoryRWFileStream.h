#pragma once

#include "FileStream.h"
#include "Vector.h"

namespace expanse
{
	struct IAllocator;

	class MemoryRWFileStream final : public FileStream
	{
	public:
		explicit MemoryRWFileStream(IAllocator *alloc);

		Result SeekStart(UFilePos_t pos) override;
		Result SeekCurrent(FilePos_t pos) override;
		Result SeekEnd(FilePos_t pos) override;

		ResultRV<UFilePos_t> GetPosition() const override;
		ResultRV<UFilePos_t> GetSize() const override;

		bool IsReadable() override;
		bool IsWriteable() override;

		ResultRV<ArrayPtr<uint8_t>> ContentsToArray() const;

	protected:
		ResultRV<size_t> Read(void *buffer, size_t size) override;
		ResultRV<size_t> Write(const void *buffer, size_t size) override;

		Vector<uint8_t> m_contents;
		size_t m_position;
	};
}
