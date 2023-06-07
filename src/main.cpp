#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "dxguid.lib")

#include "RE/CustomRE.h"

#include "Cirero.h"
#include "Helpers/DebugHelper.h"
#include "Hooks/InputManager.h"
#include "Hooks/RendererManager.h"
#include "Hooks/SKSEManager.h"
#include "Hooks/WindowsManager.h"

#include "Config.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kInputLoaded:
			Hooks::SKSEManager::GetSingleton()->Install();
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
	//#ifndef NDEBUG
	//	while (!IsDebuggerPresent()) {
	//		Sleep(100);
	//	}
	//#endif

	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());

	SKSE::Init(a_skse);

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	if (REL::Module::IsSE()) {
		ERROR("Skyrim Input Method is for 1.6.x");
		return false;
	}

	if (!SKSE::GetMessagingInterface()->RegisterListener(MessageHandler)) {
		return false;
	}

	Hooks::RendererManager::GetSingleton()->Install();
	Hooks::InputManager::GetSingleton()->Install();
	Hooks::WindowsManager::GetSingleton()->Install();
	Cicero::GetSingleton()->SetupSinks();
	auto configs = Configs::GetSingleton();
	configs->Load();

#ifdef NDEBUG
	if (configs->GetDebugMode()) {
#endif
		DebugHelper::GetSingleton()->Install();
#ifdef NDEBUG
		dku::Logger::EnableDebug(true);
	} else {
		dku::Logger::EnableDebug(false);
	}
#endif
	return true;
}
