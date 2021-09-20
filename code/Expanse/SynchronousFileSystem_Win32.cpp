#include "SynchronousFileSystem_Win32.h"

#include "CorePtr.h"
#include "FileStream_Win32.h"
#include "Result.h"
#include "ResultRV.h"
#include "StrUtils.h"
#include "XString.h"
#include "WindowsUtils.h"

#include <cstring>
#include <fileapi.h>
#include <winerror.h>

namespace expanse
{
	SynchronousFileSystem_Win32::SynchronousFileSystem_Win32()
	{
	}

	ResultRV<CorePtr<FileStream>> SynchronousFileSystem_Win32::Open(const UTF8StringView_t &device, const UTF8StringView_t &path, Permission permission, CreationDisposition creationDisposition)
	{
		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(CorePtr<FileStream_Win32>, fileStream, New<FileStream_Win32>(alloc));
		CHECK_RV(ArrayPtr<wchar_t>, canonicalPath, CanonicalizePath(device, path));

		if (canonicalPath == nullptr)
			return ErrorCode::kInvalidPath;

		bool readable = false;
		bool writeable = false;

		DWORD access = 0;
		switch (permission)
		{
		case Permission::kRead:
			access = GENERIC_READ;
			readable = true;
			break;
		case Permission::kWrite:
			access = GENERIC_WRITE;
			writeable = true;
			break;
		case Permission::kReadWrite:
			access = (GENERIC_READ | GENERIC_WRITE);
			readable = true;
			writeable = true;
			break;
		default:
			return ErrorCode::kInvalidArgument;
		}

		DWORD dwCreationDisposition = 0;
		switch (creationDisposition)
		{
		case CreationDisposition::kCreateAlways:
			dwCreationDisposition = CREATE_ALWAYS;
			break;
		case CreationDisposition::kCreateNew:
			dwCreationDisposition = CREATE_NEW;
			break;
		case CreationDisposition::kOpenAlways:
			dwCreationDisposition = OPEN_ALWAYS;
			break;
		case CreationDisposition::kOpenExisting:
			dwCreationDisposition = OPEN_EXISTING;
			break;
		case CreationDisposition::kTruncateExisting:
			dwCreationDisposition = TRUNCATE_EXISTING;
			break;
		default:
			return ErrorCode::kInvalidArgument;
		}

		HANDLE fileHandle = CreateFileW(&canonicalPath[0], access, 0, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			DWORD errCode = GetLastError();
			if (errCode == ERROR_FILE_NOT_FOUND || errCode == ERROR_PATH_NOT_FOUND)
				return ErrorCode::kFileNotFound;
			else
				return ErrorCode::kIOError;
		}

		fileStream->Init(fileHandle, readable, writeable);

		return CorePtr<FileStream>(std::move(fileStream));
	}

	Result SynchronousFileSystem_Win32::SetGamePath(const UTF8StringView_t &gamePath)
	{
		IAllocator *alloc = GetCoreObjectAllocator();

		if (gamePath.Length() > 0)
		{
			CHECK_RV(UTF8String_t, pathCopy, gamePath.CloneToString(alloc));

			m_gamePath = std::move(pathCopy);

			const uint8_t lastChar = gamePath.GetChars()[gamePath.Length() - 1];
			if (lastChar != '/' && lastChar != '\\')
			{
				CHECK(StrUtils::Append(alloc, m_gamePath, UTF8StringView_t("\\")));
			}
		}

		return ErrorCode::kOK;
	}

	ResultRV<ArrayPtr<wchar_t>> SynchronousFileSystem_Win32::CanonicalizePath(const UTF8StringView_t &device, const UTF8StringView_t &path)
	{
		const UTF8String_t *basePath = nullptr;
		if (device == UTF8StringView_t("game"))
			basePath = &m_gamePath;

		const size_t pathLen = path.Length();
		const ArrayView<const uint8_t> pathChars = path.GetChars();

		if (!basePath)
			return ArrayPtr<wchar_t>(nullptr);

		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(UTF8String_t, pathUTF8, basePath->Clone(alloc));
		CHECK(StrUtils::Append(alloc, pathUTF8, path));

		CHECK_RV(ArrayPtr<wchar_t>, converted, WindowsUtils::ConvertToWideChar(alloc, pathUTF8));

		ArrayView<wchar_t> convertedView = converted;
		for (size_t i = 0; i < convertedView.Size(); i++)
		{
			if (convertedView[i] == L'/')
				convertedView[i] = L'\\';
		}

		if (converted.Count() > 32767)
			return ErrorCode::kInvalidPath;

		return converted;
	}
}
