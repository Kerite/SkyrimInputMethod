#include "ScaleformManager.h"

// Process MenuOpenCloseEvent for determine console open state
RE::BSEventNotifyControl ScaleformManager::ProcessEvent(const RE::MenuOpenCloseEvent* a_pEvent, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_pEventSource)
{
	if (a_pEvent->menuName == RE::Console::MENU_NAME) {
		if (a_pEvent->opening) {
			InterlockedExchange(&bConsoleOpenState, TRUE);
		} else {
			InterlockedExchange(&bConsoleOpenState, FALSE);
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}

void ScaleformManager::CopyText()
{
	RE::InterfaceStrings* pInterfaceStrings = RE::InterfaceStrings::GetSingleton();
	auto pTopMovie = RE::UI::GetSingleton()->GetMovieView(pInterfaceStrings->topMenu);
}