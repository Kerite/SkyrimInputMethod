#pragma once

#include "DKUtil/Config.hpp"
#include "SimpleIni.h"

class Configs : public Singleton<Configs>
{
public:
	void Load() noexcept;

	std::wstring GetFont()
	{
		return std::wstring(ini.GetValue(TEXT("Font"), TEXT("name"), TEXT("Microsoft YaHei")));
	}

	float GetHeaderFontSize()
	{
		return ini.GetDoubleValue(L"Font", L"fHeader-size", 18.0f);
	}

	float GetInputFontSize()
	{
		return ini.GetDoubleValue(L"Font", L"input-size", 15.0f);
	}

	float GetCandidateFontSize()
	{
		return ini.GetDoubleValue(L"Font", L"candidate-size", 16.0f);
	}

	float GetX()
	{
		return ini.GetDoubleValue(L"Position", L"fX", 30.0f);
	}

	float GetY()
	{
		return ini.GetDoubleValue(L"Position", L"fY", 35.0f);
	}

	bool GetDebugMode()
	{
		return ini.GetBoolValue(L"General", L"debug", false);
	}

	int GetCandidateSize()
	{
		int candidateSize = ini.GetLongValue(L"General", L"candidate-size", 8);
		return candidateSize < 0 ? 8 : candidateSize;
	}

	bool GetUnlockWinKey()
	{
		return ini.GetBoolValue(L"Features", L"unlock-win-key", false);
	}

	bool GetPaste()
	{
		return ini.GetBoolValue(L"Features", L"paste", true);
	}

	bool bAllowPasteInConsole = true;

private:
	CSimpleIni ini;
};