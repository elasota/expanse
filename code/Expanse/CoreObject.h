#pragma once

namespace expanse
{
	class ObjectAllocator;
	struct IAllocator;

	class CoreObject
	{
	public:
		CoreObject();
		virtual ~CoreObject() {}

	protected:
		IAllocator *GetCoreObjectAllocator() const;

	private:
		friend class ObjectAllocator;

		IAllocator *m_allocator;
	};
}

namespace expanse
{
	inline CoreObject::CoreObject()
		: m_allocator(nullptr)
	{
	}

	inline IAllocator *CoreObject::GetCoreObjectAllocator() const
	{
		return m_allocator;
	}
}
