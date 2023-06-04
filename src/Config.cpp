#include "Config.h"

void Configs::Load() noexcept
{
	static auto MainConfig = COMPILE_PROXY("SkyrimChineseInput.toml"sv);

	MainConfig.Bind(mCandidateSize, 5);

	MainConfig.Load();
}