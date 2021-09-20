#include "PreprocessorLogicStack.h"

namespace expanse
{
	namespace cc
	{
		PreprocessorLogicStack::PreprocessorLogicStack()
			: m_state(PreprocessorLogicState::kDisabled)
			, m_elseEncountered(false)
		{
		}

		PreprocessorLogicStack::PreprocessorLogicStack(const FileCoordinate &coord, PreprocessorLogicState state)
			: m_coord(coord)
			, m_state(state)
			, m_elseEncountered(false)
		{
		}
	}
}
