#ifndef GameUIRendererUtilsH
#define GameUIRendererUtilsH

#include <System.SysUtils.hpp>

namespace NeonGameUIRendererUtils
{
	inline UnicodeString FormatMinutesSeconds(int totalSeconds)
	{
		const int minutes = totalSeconds / 60;
		const int seconds = totalSeconds % 60;
		return IntToStr(minutes) + L":" + (seconds < 10 ? L"0" : L"") + IntToStr(seconds);
	}
}

#endif
