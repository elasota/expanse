#include "FileCache.h"

namespace expanse
{
	namespace cc
	{
		FileCacheEntry::FileCacheEntry(bool exists)
			: m_exists(exists)
		{
		}

		FileCacheEntry::~FileCacheEntry()
		{
		}

		const ArrayPtr<uint8_t> &FileCacheEntry::GetContents() const
		{
			return m_contents;
		}

		ArrayPtr<uint8_t> FileCacheEntry::TakeContents()
		{
			return ArrayPtr<uint8_t>(std::move(m_contents));
		}

		FileCache::FileCache(IAllocator *alloc)
			: m_absolutePathCache(*alloc)
			, m_includeDirsCache(*alloc)
			, m_systemDirsCache(*alloc)
		{
		}
	}
}
