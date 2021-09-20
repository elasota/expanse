#pragma once

#include "HType.h"
#include "CLinkage.h"
#include "PPTokenStr.h"

namespace expanse
{
	namespace cc
	{
		struct TokenStrView;

		struct CGlobalObjectInfo final
		{
			CGlobalObjectInfo();

			CLinkage m_linkage;
			HTypeQualified m_type;
			TokenStrView m_linkName;

			bool m_isDefined;				// Has a definition
			bool m_isDefinitionTentative;	// Definition is tentative
			bool m_isSpeculative;			// Entry is speculative (doesn't exist in file scope)
		};
	}
}
