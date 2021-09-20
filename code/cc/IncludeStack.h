#pragma once

#include "ArrayPtr.h"
#include "ArrayView.h"
#include "CoreObject.h"
#include "CorePtr.h"
#include "FileCoordinate.h"
#include "PreprocessorLogicStack.h"
#include "PPTokenStr.h"
#include "Token.h"
#include "Vector.h"
#include "XString.h"

namespace expanse
{
	class AsyncFileRequest;

	namespace cc
	{
		struct TokenStrView;

		class IncludeStack final : public CoreObject
		{
		public:
			IncludeStack(IAllocator *alloc, IncludeStack *prev, ArrayPtr<uint8_t> &&contentsToTake, UTF8String_t &&device, UTF8String_t &&path, TokenStr &&traceName);
			IncludeStack(IAllocator *alloc, IncludeStack *prev, const ArrayView<uint8_t> &contents, UTF8String_t &&device, UTF8String_t &&path, TokenStr &&traceName);

			void Append(CorePtr<IncludeStack> &&next);
			void UnlinkNext();

			IncludeStack *GetPrev() const;

			const FileCoordinate &GetFileCoordinate() const;
			void SetFileCoordinate(const FileCoordinate &coordinate);

			ArrayView<const uint8_t> GetFileContents() const;

			void GetFileName(UTF8StringView_t &outDevice, UTF8StringView_t &outPath) const;
			TokenStrView GetTraceFileName() const;

			Result PushLogic(const PreprocessorLogicStack &logic);
			void PopLogic();
			PreprocessorLogicStack *GetTopLogic();
			bool IsInActivePreprocessorBlock() const;

		private:
			CorePtr<IncludeStack> m_next;
			ArrayPtr<uint8_t> m_ownedContents;
			ArrayView<uint8_t> m_contents;
			CorePtr<AsyncFileRequest> m_asyncFileRequest;
			Vector<PreprocessorLogicStack> m_logicStack;

			IncludeStack *m_prev;

			UTF8String_t m_device;
			UTF8String_t m_path;
			TokenStr m_traceName;

			FileCoordinate m_coordinate;
		};
	}
}
