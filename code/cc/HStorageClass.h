#pragma once

namespace expanse
{
	namespace cc
	{
		enum class HStorageClass
		{
			kInvalid,

			kTypeDef,
			kExtern,
			kStatic,
			kAuto,
			kRegister,
		};
	}
}
