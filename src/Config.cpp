#include "Config.h"

void Configs::Load() noexcept
{
	ini.LoadFile("Data\\SKSE\\Plugins\\SkyrimChineseInput.ini");
}