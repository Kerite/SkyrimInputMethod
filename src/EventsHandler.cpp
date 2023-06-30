#include "EventsHandler.h"

void EventsHandler::Install()
{
	INFO("Installing EventsHandler");
	if (m_bInitialized)
		return;
	RE::UI* pUI = RE::UI::GetSingleton();
	if (!pUI) {
		return;
	}
	INFO("Registering MenuOpenCloseEvent event");
	pUI->AddEventSink<RE::MenuOpenCloseEvent>(this);
	m_bInitialized = true;
	INFO("Installed EventsHandler");
}

// Process MenuOpenCloseEvent for determine console open state
RE::BSEventNotifyControl EventsHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_pEvent, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_pEventSource)
{
	if (a_pEvent->menuName == RE::Console::MENU_NAME) {
		INFO("Processing Console OpenCloseEvent");
		if (a_pEvent->opening) {
			InterlockedExchange(&bConsoleOpenState, TRUE);
		} else {
			InterlockedExchange(&bConsoleOpenState, FALSE);
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}

void EventsHandler::CopyText()
{
	RE::InterfaceStrings* pInterfaceStrings = RE::InterfaceStrings::GetSingleton();
	auto pTopMovie = RE::UI::GetSingleton()->GetMovieView(pInterfaceStrings->topMenu);
}