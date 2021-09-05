#pragma once

#include "SynchronousFileSystem.h"
#include "StringProto.h"
#include "String.h"

namespace expanse
{
	struct Result;
	template<class T> struct ArrayPtr;
	template<class T> struct ResultRV;

	class SynchronousFileSystem_Win32 : public SynchronousFileSystem
	{
	public:
		ResultRV<CorePtr<FileStream>> Open(const UTF8StringView_t &device, const UTF8StringView_t &path, Permission permission, CreationDisposition creationDisposition) override;

		Result SetGamePath(const UTF8StringView_t &path);

	private:
		ResultRV<ArrayPtr<wchar_t>> CanonicalizePath(const UTF8StringView_t &device, const UTF8StringView_t &path);

		UTF8String_t m_gamePath;
	};
}
