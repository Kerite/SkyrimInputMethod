#pragma once

#include "DKUtil/Config.hpp"
#include "SimpleIni.h"

class Configs : public Singleton<Configs>
{
public:
	static inline int32_t iCandidateSize{ 8 };
	static inline bool bDebug{ false };

	static inline bool bFeaturePaste{ true };
	static inline bool bFeatureUnlockWinKey{ false };

	static inline std::string sFontPath{ "msyh.ttc" };
	static inline float fFontSize{ 20.f };

	static inline float fPositionX{ 30.f };
	static inline float fPositionY{ 30.f };

	bool bAllowPasteInConsole = true;

	void Load() noexcept;
};