#pragma once

namespace expanse
{
	template<class T> struct ArrayPtr;
	class CoreObject;
	template<class T> struct CorePtr;
	struct CorePtrBase;
	template<class T> struct ResultRV;
	struct IAllocator;

	class ObjectAllocator
	{
	public:
		template<class T, class ...TArgs>
		static CorePtr<T> New(IAllocator *alloc, TArgs&&...);

		template<class T>
		static CorePtr<T> New(IAllocator *alloc);

		template<class T>
		static ArrayPtr<T> NewArray(IAllocator *alloc, size_t numElements);

		template<class T>
		static ArrayPtr<T> NewArrayUninitialized(IAllocator *alloc, size_t numElements);

		template<class T>
		static ArrayPtr<T> NewArray(IAllocator *alloc, size_t numElements, const T &initializer);

		template<class T>
		static void DeleteArray(ArrayPtr<T> &ptr);

		static void DeleteObject(CoreObject *obj);
	};

	template<class T>
	CorePtr<T> TryNew(IAllocator *alloc);

	template<class T>
	ArrayPtr<T> TryNewArray(IAllocator *alloc, size_t numElements);

	template<class T>
	ArrayPtr<T> TryNewArray(IAllocator *alloc, size_t numElements, const T &initializer);

	template<class T>
	ResultRV<CorePtr<T>> New(IAllocator *alloc);

	template<class T, class ...TArgs>
	ResultRV<CorePtr<T>> New(IAllocator *alloc, TArgs &&...args);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(IAllocator *alloc, size_t numElements);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArrayUninitialized(IAllocator *alloc, size_t numElements);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(IAllocator *alloc, size_t numElements, const T &initializer);
}

#include "ArrayPtr.h"
#include "CorePtr.h"
#include "CoreObject.h"
#include "IAllocator.h"
#include "ResultRV.h"

#include <utility>
#include <new>

namespace expanse
{
	template<class T, class ...TArgs>
	CorePtr<T> ObjectAllocator::New(IAllocator *alloc, TArgs&& ...args)
	{
		void *mem = alloc->Alloc(sizeof(T), alignof(T));
		if (mem == nullptr)
			return nullptr;

		CoreObject *obj = new (mem) T(std::forward<TArgs>(args)...);
		obj->m_allocator = alloc;
		return CorePtr<T>(static_cast<T*>(obj));
	}

	template<class T>
	CorePtr<T> ObjectAllocator::New(IAllocator *alloc)
	{
		void *mem = alloc->Alloc(sizeof(T), alignof(T));
		if (mem == nullptr)
			return nullptr;

		CoreObject *obj = new (mem) T();
		obj->m_allocator = alloc;
		return CorePtr<T>(static_cast<T*>(obj));
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArray(IAllocator *alloc, size_t numElements)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = alloc->Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);
		for (size_t i = 0; i < numElements; i++)
			new (arr + i) T();

		return ArrayPtr<T>(alloc, arr, numElements);
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArrayUninitialized(IAllocator *alloc, size_t numElements)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = alloc->Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);

		return ArrayPtr<T>(alloc, arr, numElements);
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArray(IAllocator *alloc, size_t numElements, const T &initializer)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = alloc->Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);
		for (size_t i = 0; i < numElements; i++)
			new (arr + i) T(initializer);

		return ArrayPtr<T>(alloc, arr, numElements);
	}


	template<class T>
	void ObjectAllocator::DeleteArray(ArrayPtr<T> &ptr)
	{
		T *elements = ptr.m_elements;
		if (elements == nullptr)
			return;

		const size_t size = ptr.m_size;
		for (size_t i = 0; i < size; i++)
			elements[(size - 1) - i].~T();

		ptr.m_alloc->Release(elements);
	}

	template<class T>
	CorePtr<T> TryNew(IAllocator *alloc)
	{
		return ObjectAllocator::New<T>(alloc);
	}


	template<class T>
	ArrayPtr<T> TryNewArray(IAllocator *alloc, size_t numElements)
	{
		return ObjectAllocator::NewArray<T>(alloc, numElements);
	}

	template<class T>
	ArrayPtr<T> TryNewArray(IAllocator *alloc, size_t numElements, const T &initializer)
	{
		return ObjectAllocator::NewArray<T>(numElements, initializer);
	}

	template<class T>
	ResultRV<CorePtr<T>> New(IAllocator *alloc)
	{
		CorePtr<T> result(ObjectAllocator::New<T>(alloc));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T, class ...TArgs>
	ResultRV<CorePtr<T>> New(IAllocator *alloc, TArgs &&...args)
	{
		CorePtr<T> result(ObjectAllocator::New<T, TArgs...>(alloc, std::forward<TArgs>(args)...));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(IAllocator *alloc, size_t numElements)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArray<T>(alloc, numElements));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArrayUninitialized(IAllocator *alloc, size_t numElements)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArrayUninitialized<T>(alloc, numElements));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(IAllocator *alloc, size_t numElements, const T &initializer)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArray<T>(alloc, numElements, initializer));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}
}
