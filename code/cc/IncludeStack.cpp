#include "IncludeStack.h"

#include "CoreObject.h"
#include "CorePtr.h"

expanse::cc::IncludeStack::IncludeStack(IAllocator *alloc, IncludeStack *prev, ArrayPtr<uint8_t> &&contentsToTake, UTF8String_t &&device, UTF8String_t &&path, TokenStr &&traceName)
	: m_prev(prev)
	, m_ownedContents(std::move(contentsToTake))
	, m_device(std::move(device))
	, m_path(std::move(path))
	, m_coordinate(0, 1, 0)
	, m_logicStack(alloc)
	, m_traceName(std::move(traceName))
{
	m_contents = ArrayView<uint8_t>(m_ownedContents);
}

expanse::cc::IncludeStack::IncludeStack(IAllocator *alloc, IncludeStack *prev, const ArrayView<uint8_t> &contents, UTF8String_t &&device, UTF8String_t &&path, TokenStr &&traceName)
	: m_prev(prev)
	, m_contents(contents)
	, m_device(std::move(device))
	, m_path(std::move(path))
	, m_coordinate(0, 1, 0)
	, m_logicStack(alloc)
	, m_traceName(std::move(traceName))
{
}

void expanse::cc::IncludeStack::Append(CorePtr<IncludeStack> &&next)
{
	IncludeStack *nextPtr = next;

	m_next = std::move(next);
	m_next->m_prev = this;
}

void expanse::cc::IncludeStack::UnlinkNext()
{
	if (m_next != nullptr)
	{
		m_next->m_prev = nullptr;
		m_next = nullptr;
	}
}

expanse::cc::IncludeStack *expanse::cc::IncludeStack::GetPrev() const
{
	return m_prev;
}


const expanse::cc::FileCoordinate &expanse::cc::IncludeStack::GetFileCoordinate() const
{
	return m_coordinate;
}

void expanse::cc::IncludeStack::SetFileCoordinate(const FileCoordinate &coordinate)
{
	m_coordinate = coordinate;
}

expanse::ArrayView<const uint8_t> expanse::cc::IncludeStack::GetFileContents() const
{
	return ArrayView<const uint8_t>(m_contents);
}

void expanse::cc::IncludeStack::GetFileName(UTF8StringView_t &outDevice, UTF8StringView_t &outPath) const
{
	outDevice = m_device;
	outPath = m_path;
}

expanse::cc::TokenStrView expanse::cc::IncludeStack::GetTraceFileName() const
{
	return m_traceName.GetTokenView();
}

expanse::Result expanse::cc::IncludeStack::PushLogic(const PreprocessorLogicStack &logic)
{
	CHECK(m_logicStack.Add(logic));
	return ErrorCode::kOK;
}

void expanse::cc::IncludeStack::PopLogic()
{
	EXP_ASSERT(m_logicStack.Size() > 0);
	m_logicStack.Resize(m_logicStack.Size() - 1);
}

expanse::cc::PreprocessorLogicStack *expanse::cc::IncludeStack::GetTopLogic()
{
	if (m_logicStack.Size() == 0)
		return nullptr;

	return &m_logicStack[m_logicStack.Size() - 1];
}

bool expanse::cc::IncludeStack::IsInActivePreprocessorBlock() const
{
	if (m_logicStack.Size() == 0)
		return true;

	return m_logicStack[m_logicStack.Size() - 1].m_state == PreprocessorLogicState::kActive;
}
