#pragma once

#include "FileCoordinate.h"

namespace expanse
{
	namespace cc
	{
		enum class PreprocessorLogicState
		{
			kActive,		// Currently in an active block
			kNotYetActive,	// Currently in an inactive block nested within an active block, so an else/elif block may cause it to become active
			kDisabled,		// Either was never active, or can no longer be active
		};

		struct PreprocessorLogicStack
		{
			PreprocessorLogicStack();
			PreprocessorLogicStack(const FileCoordinate &coord, PreprocessorLogicState state);

			FileCoordinate m_coord;
			PreprocessorLogicState m_state;
			bool m_elseEncountered;
		};
	}
}
