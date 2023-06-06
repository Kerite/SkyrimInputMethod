#include "Config.h"

void Configs::Load() noexcept
{
	ini.LoadFile("Data\\SKSE\\Plugins\\SkyrimInputMethod.ini");
}