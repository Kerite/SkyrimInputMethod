#include "RE/CustomRE.h"

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

	//REL::Module::reset();
	SKSE::Init(a_skse);

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// do stuff
	if (!SKSE::GetMessagingInterface()->RegisterListener(MessageHandler)) {
		return false;
	}
#ifndef NDEBUG
	DebugHelper::GetSingleton()->Install();
#endif
	Hooks::RendererManager::GetSingleton()->Install();
	Hooks::InputManager::GetSingleton()->Install();
	Hooks::WindowsManager::GetSingleton()->Install();
	Configs::GetSingleton()->Load();

	return true;
}
