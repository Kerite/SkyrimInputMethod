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
		return ini.GetDoubleValue(L"Font", L"header-size", 18.0f);
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
		return ini.GetDoubleValue(L"Position", L"x", 30.0f);
	}

	float GetY()
	{
		return ini.GetDoubleValue(L"Position", L"y", 35.0f);
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

private:
	CSimpleIni ini;
};