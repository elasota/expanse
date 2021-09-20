#pragma once

#include "FileStream.h"

#include "IncludeWindows.h"

namespace expanse
{
	class SynchronousFileSystem_Win32;

	class FileStream_Win32 final : public FileStream
	{
	public:
		FileStream_Win32();
		~FileStream_Win32();

		Result SeekStart(UFilePos_t pos) override;
		Result SeekCurrent(FilePos_t pos) override;
		Result SeekEnd(FilePos_t pos) override;

		ResultRV<UFilePos_t> GetPosition() const override;
		ResultRV<UFilePos_t> GetSize() const override;

		bool IsReadable() override;
		bool IsWriteable() override;

	protected:
		ResultRV<size_t> Read(void *buffer, size_t size) override;
		ResultRV<size_t> Write(const void *buffer, size_t size) override;

	private:
		friend class SynchronousFileSystem_Win32;

		void Init(HANDLE handle, bool readable, bool writeable);

		HANDLE m_handle;
		bool m_isReadable;
		bool m_isWriteable;
	};
}
