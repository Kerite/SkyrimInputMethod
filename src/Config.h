#pragma once

#include "DKUtil/Config.hpp"
#include "SimpleIni.h"

class Configs : public Singleton<Configs>
{
public:
	static inline bool bDebug{ false };
	static inline int32_t iCandidateSize{ 8 };

	static inline std::string sFontPath{ "msyh.ttc" };
	static inline float fFontSize{ 20.f };

	static inline float fPositionX{ 30.f };
	static inline float fPositionY{ 30.f };

	static inline bool bFeaturePaste{ true };
	static inline bool bFeatureUnlockWinKey{ false };

	bool bAllowPasteInConsole = true;

	void Load() noexcept;
};