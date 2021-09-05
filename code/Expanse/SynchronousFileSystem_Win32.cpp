#include "SynchronousFileSystem_Win32.h"

#include "CorePtr.h"
#include "FileStream_Win32.h"
#include "Result.h"
#include "ResultRV.h"
#include "StrUtils.h"
#include "String.h"
#include "WindowsUtils.h"

#include <cstring>
#include <fileapi.h>
#include <winerror.h>

namespace expanse
{
	ResultRV<CorePtr<FileStream>> SynchronousFileSystem_Win32::Open(const UTF8StringView_t &device, const UTF8StringView_t &path, Permission permission, CreationDisposition creationDisposition)
	{
		CHECK_RV(CorePtr<FileStream_Win32>, fileStream, New<FileStream_Win32>());
		CHECK_RV(ArrayPtr<wchar_t>, canonicalPath, CanonicalizePath(device, path));

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
			return ErrorCode::kIOError;

		fileStream->Init(fileHandle, readable, writeable);

		return CorePtr<FileStream>(std::move(fileStream));
	}

	Result SynchronousFileSystem_Win32::SetGamePath(const UTF8StringView_t &gamePath)
	{
		CHECK_RV(UTF8String_t, pathCopy, gamePath.CloneToString());
		m_gamePath = std::move(pathCopy);

		return ErrorCode::kOK;
	}

	ResultRV<ArrayPtr<wchar_t>> SynchronousFileSystem_Win32::CanonicalizePath(const UTF8StringView_t &device, const UTF8StringView_t &path)
	{
		const UTF8String_t *basePath = nullptr;
		if (device == UTF8StringView_t("game"))
			basePath = &m_gamePath;

		if (!basePath)
			return ArrayPtr<wchar_t>(nullptr);

		CHECK_RV(UTF8String_t, pathUTF8, basePath->Clone());
		CHECK(StrUtils::Append(pathUTF8, UTF8StringView_t("\\")));
		CHECK(StrUtils::Append(pathUTF8, path));

		CHECK_RV(ArrayPtr<wchar_t>, converted, WindowsUtils::ConvertToWideChar(pathUTF8));

		if (converted.Count() > 32767)
			return ErrorCode::kInvalidPath;

		return converted;
	}
}
