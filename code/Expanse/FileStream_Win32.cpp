#include "FileStream_Win32.h"
#include "WindowsUtils.h"
#include "Result.h"

namespace expanse
{
	FileStream_Win32::FileStream_Win32()
		: m_handle(nullptr)
		, m_isReadable(false)
		, m_isWriteable(false)
	{
	}

	FileStream_Win32::~FileStream_Win32()
	{
		if (m_handle)
			CloseHandle(m_handle);
	}

	Result FileStream_Win32::SeekStart(UFilePos_t pos)
	{
		if (pos >= static_cast<UFilePos_t>(std::numeric_limits<int64_t>::max()))
			return ErrorCode::kInvalidArgument;

		if (!SetFilePointerEx(m_handle, WindowsUtils::Int64ToLargeInteger(static_cast<int64_t>(pos)), nullptr, FILE_BEGIN))
			return ErrorCode::kIOError;

		return ErrorCode::kOK;
	}

	Result FileStream_Win32::SeekCurrent(FilePos_t pos)
	{
		if (!SetFilePointerEx(m_handle, WindowsUtils::Int64ToLargeInteger(pos), nullptr, FILE_CURRENT))
			return ErrorCode::kIOError;

		return ErrorCode::kOK;
	}

	ResultRV<UFilePos_t> FileStream_Win32::GetPosition() const
	{
		LARGE_INTEGER distanceToMove = WindowsUtils::Int64ToLargeInteger(0);
		LARGE_INTEGER newFilePointer;
		if (!SetFilePointerEx(m_handle, distanceToMove, &newFilePointer, FILE_CURRENT))
			return ErrorCode::kIOError;

		return static_cast<UFilePos_t>(WindowsUtils::LargeIntegerToInt64(newFilePointer));
	}

	ResultRV<UFilePos_t> FileStream_Win32::GetSize() const
	{
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(m_handle, &fileSize))
			return ErrorCode::kIOError;

		return static_cast<UFilePos_t>(WindowsUtils::LargeIntegerToInt64(fileSize));
	}

	Result FileStream_Win32::SeekEnd(FilePos_t pos)
	{
		if (!SetFilePointerEx(m_handle, WindowsUtils::Int64ToLargeInteger(pos), nullptr, FILE_END))
			return ErrorCode::kIOError;

		return ErrorCode::kOK;
	}

	bool FileStream_Win32::IsReadable()
	{
		return m_isReadable;
	}

	bool FileStream_Win32::IsWriteable()
	{
		return m_isWriteable;
	}

	ResultRV<size_t> FileStream_Win32::Read(void *buffer, size_t size)
	{
		size_t cumulativeRead = 0;
		while (size > MAXDWORD)
		{
			DWORD numberRead = 0;
			if (!ReadFile(m_handle, buffer, MAXDWORD, &numberRead, nullptr))
				return ErrorCode::kIOError;

			cumulativeRead += static_cast<size_t>(numberRead);
			if (numberRead < MAXDWORD)
				return cumulativeRead;

			buffer = static_cast<void*>(static_cast<uint8_t*>(buffer) + numberRead);
			size -= static_cast<size_t>(numberRead);
		}

		DWORD numberRead = 0;
		if (!ReadFile(m_handle, buffer, static_cast<DWORD>(size), &numberRead, nullptr))
			return ErrorCode::kIOError;

		return cumulativeRead + static_cast<size_t>(numberRead);
	}

	ResultRV<size_t> FileStream_Win32::Write(const void *buffer, size_t size)
	{
		size_t cumulativeWritten = 0;
		while (size > MAXDWORD)
		{
			DWORD numberWritten = 0;
			if (!WriteFile(m_handle, buffer, MAXDWORD, &numberWritten, nullptr))
				return ErrorCode::kIOError;

			cumulativeWritten += static_cast<size_t>(numberWritten);
			if (numberWritten < MAXDWORD)
				return cumulativeWritten;

			buffer = static_cast<const void*>(static_cast<const uint8_t*>(buffer) + numberWritten);
			size -= static_cast<size_t>(numberWritten);
		}

		DWORD numberWritten = 0;
		if (!WriteFile(m_handle, buffer, static_cast<DWORD>(size), &numberWritten, nullptr))
			return ErrorCode::kIOError;

		return cumulativeWritten + static_cast<size_t>(numberWritten);
	}

	void FileStream_Win32::Init(HANDLE handle, bool readable, bool writeable)
	{
		m_handle = handle;
		m_isReadable = readable;
		m_isWriteable = writeable;
	}
}
