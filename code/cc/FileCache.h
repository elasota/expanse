#pragma once

#include "CoreObject.h"
#include "HashMap.h"
#include "XString.h"

namespace expanse
{
	namespace cc
	{
		struct FileCacheEntry final
		{
		public:
			explicit FileCacheEntry(bool exists);
			~FileCacheEntry();

			bool Exists() const;

			const ArrayPtr<uint8_t> &GetContents() const;
			ArrayPtr<uint8_t> TakeContents();

		private:
			bool m_exists;
			ArrayPtr<uint8_t> m_contents;
		};

		class FileCache final : public CoreObject
		{
		public:
			explicit FileCache(IAllocator *alloc);

		private:
			HashMap<UTF8String_t, FileCacheEntry> m_absolutePathCache;
			HashMap<UTF8String_t, FileCacheEntry> m_includeDirsCache;
			HashMap<UTF8String_t, FileCacheEntry> m_systemDirsCache;
		};
	}
}
