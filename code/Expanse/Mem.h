#pragma once

namespace expanse
{
	template<class T> struct ArrayPtr;
	class CoreObject;
	template<class T> struct CorePtr;
	struct CorePtrBase;
	template<class T> struct ResultRV;

	class Mem
	{
	public:
		static void *Alloc(size_t size, size_t alignment);
		static void Release(void *ptr);
		static void *Realloc(void *ptr, size_t newSize, size_t alignment);
	};

	class ObjectAllocator
	{
	public:
		template<class T, class ...TArgs>
		static CorePtr<T> New(TArgs&&...);

		template<class T>
		static CorePtr<T> New();

		template<class T>
		static ArrayPtr<T> NewArray(size_t numElements);

		template<class T>
		static ArrayPtr<T> NewArrayUninitialized(size_t numElements);

		template<class T>
		static ArrayPtr<T> NewArray(size_t numElements, const T &initializer);

		static void Delete(CorePtrBase &ptr);

		template<class T>
		static void DeleteArray(ArrayPtr<T> &ptr);
	};

	template<class T>
	CorePtr<T> TryNew();

	template<class T>
	ArrayPtr<T> TryNewArray(size_t numElements);

	template<class T>
	ArrayPtr<T> TryNewArray(size_t numElements, const T &initializer);

	template<class T>
	ResultRV<CorePtr<T>> New();

	template<class T, class ...TArgs>
	ResultRV<CorePtr<T>> New(TArgs &&...args);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(size_t numElements);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArrayUninitialized(size_t numElements);

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(size_t numElements, const T &initializer);
}

#include "ArrayPtr.h"
#include "CorePtr.h"
#include "CoreObject.h"
#include "ResultRV.h"

#include <utility>
#include <new>

namespace expanse
{
	template<class T, class ...TArgs>
	CorePtr<T> ObjectAllocator::New(TArgs&& ...args)
	{
		void *mem = Mem::Alloc(sizeof(T), alignof(T));
		if (mem == nullptr)
			return nullptr;

		CoreObject *obj = new (mem) T(std::forward<TArgs>(args)...);
		return CorePtr<T>(static_cast<T*>(obj));
	}

	template<class T>
	CorePtr<T> ObjectAllocator::New()
	{
		void *mem = Mem::Alloc(sizeof(T), alignof(T));
		if (mem == nullptr)
			return nullptr;

		CoreObject *obj = new (mem) T();
		return CorePtr<T>(static_cast<T*>(obj));
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArray(size_t numElements)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = Mem::Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);
		for (size_t i = 0; i < numElements; i++)
			new (arr + i) T();

		return ArrayPtr<T>(arr, numElements);
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArrayUninitialized(size_t numElements)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = Mem::Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);

		return ArrayPtr<T>(arr, numElements);
	}

	template<class T>
	ArrayPtr<T> ObjectAllocator::NewArray(size_t numElements, const T &initializer)
	{
		if (numElements == 0)
			return nullptr;

		const size_t maxElements = std::numeric_limits<size_t>::max() / sizeof(T);
		if (numElements > maxElements)
			return nullptr;

		void *mem = Mem::Alloc(sizeof(T) * numElements, alignof(T));
		T *arr = static_cast<T*>(mem);
		for (size_t i = 0; i < numElements; i++)
			new (arr + i) T(initializer);

		return ArrayPtr<T>(arr, numElements);
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

		Mem::Release(elements);
	}

	template<class T>
	CorePtr<T> TryNew()
	{
		return ObjectAllocator::New<T>();
	}


	template<class T>
	ArrayPtr<T> TryNewArray(size_t numElements)
	{
		return ObjectAllocator::NewArray<T>(numElements);
	}

	template<class T>
	ArrayPtr<T> TryNewArray(size_t numElements, const T &initializer)
	{
		return ObjectAllocator::NewArray<T>(numElements, initializer);
	}

	template<class T>
	ResultRV<CorePtr<T>> New()
	{
		CorePtr<T> result(ObjectAllocator::New<T>());
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T, class ...TArgs>
	ResultRV<CorePtr<T>> New(TArgs &&...args)
	{
		CorePtr<T> result(ObjectAllocator::New<T, TArgs...>(std::forward<TArgs>(args)...));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(size_t numElements)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArray<T>(numElements));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArrayUninitialized(size_t numElements)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArrayUninitialized<T>(numElements));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}

	template<class T>
	ResultRV<ArrayPtr<T>> NewArray(size_t numElements, const T &initializer)
	{
		ArrayPtr<T> result(ObjectAllocator::NewArray<T>(numElements, initializer));
		if (result == nullptr)
			return ErrorCode::kOutOfMemory;

		return result;
	}
}
