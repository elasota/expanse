#pragma once

#include "SynchronousFileSystem.h"
#include "StringProto.h"
#include "XString.h"

namespace expanse
{
	struct IAllocator;
	struct Result;
	template<class T> struct ArrayPtr;
	template<class T> struct ResultRV;

	class SynchronousFileSystem_Win32 : public SynchronousFileSystem
	{
	public:
		SynchronousFileSystem_Win32();

		ResultRV<CorePtr<FileStream>> Open(const UTF8StringView_t &device, const UTF8StringView_t &path, Permission permission, CreationDisposition creationDisposition) override;

		Result SetGamePath(const UTF8StringView_t &gamePath);

	private:
		ResultRV<ArrayPtr<wchar_t>> CanonicalizePath(const UTF8StringView_t &device, const UTF8StringView_t &path);

		UTF8String_t m_gamePath;
	};
}
