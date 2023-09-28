#include "Config.h"

void ReadBoolean(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, bool& a_out)
{
	INFO("Reading [{}]{}", a_section, a_key);
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		auto value = a_ini.GetBoolValue(a_section.data(), a_key.data());
		a_out = value;
	}
}

void ReadInt32(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, int32_t& a_out)
{
	INFO("Reading [{}]{}", a_section, a_key);
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = a_ini.GetLongValue(a_section.data(), a_key.data());
	}
}

void ReadFloat(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, float& a_out)
{
	INFO("Reading [{}]{}", a_section, a_key);
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = a_ini.GetDoubleValue(a_section.data(), a_key.data());
	}
}

void ReadString(CSimpleIniA& a_ini, std::string_view a_section, std::string_view a_key, std::string& a_out)
{
	INFO("Reading [{}]{}", a_section, a_key);
	auto found = a_ini.GetValue(a_section.data(), a_key.data());
	if (found) {
		a_out = std::string(found);
	}
}

void LoadSettings(std::filesystem::path path)
{
	CSimpleIniA config;
	INFO("Loading configs form file: {}", path.string().c_str());
	config.LoadFile(path.string().c_str());

	ReadInt32(config, "General", "candidate-size", Configs::iCandidateSize);
	ReadBoolean(config, "General", "debug", Configs::bDebug);
	ReadString(config, "General", "glyph-range-source-path", Configs::sGlyphRangeSourcePath);

	ReadBoolean(config, "Feature", "unlock-win-key", Configs::bFeatureUnlockWinKey);
	ReadBoolean(config, "Feature", "paste", Configs::bFeaturePaste);

	ReadString(config, "Font", "font", Configs::sFontPath);
	ReadFloat(config, "Font", "font-size", Configs::fFontSize);

	ReadFloat(config, "Position", "x", Configs::fPositionX);
	ReadFloat(config, "Position", "y", Configs::fPositionY);
}

void Configs::Load() noexcept
{
	LoadSettings("Data\\SKSE\\Plugins\\SkyrimInputMethod.ini");
}