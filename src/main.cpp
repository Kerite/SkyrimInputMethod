#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dxguid.lib")

#include "RE/CustomRE.h"

#include "Cirero.h"
#include "EventsHandler.h"
#include "Hooks/InputManager.h"
#include "Hooks/RendererManager.h"
#include "Hooks/ScaleformManager.h"
#include "Hooks/WindowsManager.h"

#include "Config.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostPostLoad:
			if (GetModuleHandle(L"po3_ConsolePlusPlus.dll")) {
				INFO("ConsolePlusPlus detected, disable paste in console");
				Configs::GetSingleton()->bAllowPasteInConsole = false;
			}
			break;
		case SKSE::MessagingInterface::kInputLoaded:
			Hooks::ScaleformManager::GetSingleton()->Install();
			EventsHandler::GetSingleton()->Install();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			break;
		default:
			break;
		}
	}
}

DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());

	SKSE::Init(a_skse);

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);
	if (!SKSE::GetMessagingInterface()->RegisterListener(MessageHandler)) {
		return false;
	}

	Hooks::RendererManager::GetSingleton()->Install();
	Hooks::InputManager::GetSingleton()->Install();
	Cicero::GetSingleton()->SetupSinks();
	Configs::GetSingleton()->Load();

#ifdef NDEBUG
	dku::Logger::EnableDebug(Configs::bDebug);
#endif
	return true;
}
