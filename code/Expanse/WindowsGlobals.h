#pragma once

#include "IncludeWindows.h"

namespace expanse
{
	struct WindowsGlobals
	{
		HINSTANCE m_hInstance;
		HINSTANCE m_hPrevInstance;
		int m_cmdShow;

		LPWSTR *m_argv;
		int m_argc;

		static WindowsGlobals ms_instance;
	};
}
