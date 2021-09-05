#pragma once

#include "CoreObject.h"
#include "StringProto.h"

namespace expanse
{
	class FileStream;
	template<class T> struct CorePtr;
	template<class T> struct ResultRV;

	class SynchronousFileSystem : public CoreObject
	{
	public:
		enum class CreationDisposition
		{
			kCreateAlways,
			kCreateNew,
			kOpenAlways,
			kOpenExisting,
			kTruncateExisting,
		};

		enum class Permission
		{
			kRead,
			kWrite,
			kReadWrite,
		};

		virtual ResultRV<CorePtr<FileStream>> Open(const UTF8StringView_t &device, const UTF8StringView_t &path, Permission permission, CreationDisposition creationDisposition) = 0;
	};
}
