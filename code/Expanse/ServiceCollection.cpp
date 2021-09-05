#include "ServiceCollection.h"

namespace expanse
{
	ServiceCollection *ServiceCollection::ms_primaryInstance = nullptr;

	ServiceCollection::ServiceCollection()
		: m_asyncFileSystem(nullptr)
		, m_syncFileSystem(nullptr)
	{
	}
}
