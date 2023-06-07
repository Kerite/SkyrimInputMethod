#include "ScaleformManager.h"

RE::BSEventNotifyControl ScaleformManager::ProcessEvent(const RE::MenuOpenCloseEvent* a_pEvent, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_pEventSource)
{
	if (a_pEvent->menuName != RE::Console::MENU_NAME)
		return RE::BSEventNotifyControl::kContinue;

	if (a_pEvent->opening) {
		InterlockedExchange(&bConsoleOpenState, TRUE);
	}
	else {
		InterlockedExchange(&bConsoleOpenState, FALSE);
	}

	return RE::BSEventNotifyControl::kContinue;
}