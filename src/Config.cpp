#include "Config.h"

void ReadBoolean(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, bool& a_out)
{
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		auto value = a_ini.GetBoolValue(a_section.data(), a_key.data());
		a_out = value;
	}
	INFO("Read config [{}]{}, value: {}", a_section, a_key, found)
}

void ReadInt32(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, int32_t& a_out)
{
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = a_ini.GetLongValue(a_section.data(), a_key.data());
	}
	INFO("Read config [{}]{}, value: {}", a_section, a_key, found)
}

void ReadFloat(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, float& a_out)
{
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = a_ini.GetDoubleValue(a_section.data(), a_key.data());
	}
	INFO("Read config [{}]{}, value: {}", a_section, a_key, found)
}

void ReadString(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, std::string& a_out)
{
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = std::string(found);
	}
	INFO("Read config [{}]{}, value: {}", a_section, a_key, found)
}

void LoadSettings(std::filesystem::path path)
{
	CSimpleIniA config;

	ReadInt32(config, "General", "candidate-size", Configs::iCandidateSize);

	ReadString(config, "Font", "font", Configs::sFontPath);
	ReadFloat(config, "Font", "font-size", Configs::fFontSize);

	ReadFloat(config, "Position", "x", Configs::fPositionX);
	ReadFloat(config, "Position", "y", Configs::fPositionY);

	ReadBoolean(config, "Feature", "unlock-win-key", Configs::bFeatureUnlockWinKey);
	ReadBoolean(config, "Feature", "paste", Configs::bFeaturePaste);
}

void Configs::Load() noexcept
{
	LoadSettings("Data\\SKSE\\Plugins\\SkyrimInputMethod.ini");
}