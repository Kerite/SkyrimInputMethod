#include "ScaleformManager.h"

RE::BSEventNotifyControl ScaleformManager::ProcessEvent(const RE::MenuOpenCloseEvent* a_pEvent, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_pEventSource)
{
	if (a_pEvent->menuName != RE::Console::MENU_NAME)
		return RE::BSEventNotifyControl::kContinue;

	if (a_pEvent->opening) {
		InterlockedExchange(&bConsoleOpenState, TRUE);
	} else {
		InterlockedExchange(&bConsoleOpenState, FALSE);
	}

	return RE::BSEventNotifyControl::kContinue;
}

void ScaleformManager::CopyText()
{
	RE::InterfaceStrings* pInterfaceStrings = RE::InterfaceStrings::GetSingleton();
	auto pTopMovie = RE::UI::GetSingleton()->GetMovieView(pInterfaceStrings->topMenu);

	//if (bConsoleOpenState) {
	//	RE::GFxMovieView* view = RE::UI::GetSingleton()->GetMenu(RE::Console::MENU_NAME);
	//	if (view) {
	//		RE::GFxValue console;
	//		view->GetVariable(&console, "_root.Menu_mc.Console_mc");
	//		if (console.IsObject()) {
	//			RE::GFxValue text;
	//			console.GetMember("TextField", &text);
	//			if (text.IsObject()) {
	//				RE::GFxValue selection;
	//				text.GetMember("selectionEndIndex", &selection);
	//				if (selection.IsNumber()) {
	//					RE::GFxValue textValue;
	//					text.GetMember("text", &textValue);
	//					if (textValue.IsString()) {
	//						std::string str = textValue.GetString();
	//						int selectionEndIndex = selection.GetNumber();
	//						int selectionStartIndex = text.GetMember("selectionBeginIndex").GetNumber();
	//						if (selectionEndIndex > selectionStartIndex) {
	//							str = str.substr(selectionStartIndex, selectionEndIndex - selectionStartIndex);
	//						}
	//						if (!str.empty()) {
	//							OpenClipboard(NULL);
	//							EmptyClipboard();
	//							HGLOBAL hClipboardData;
	//							hClipboardData = GlobalAlloc(GMEM_DDESHARE, str.size() + 1);
	//							char* pchData = (char*)GlobalLock(hClipboardData);
	//							strcpy_s(pchData, str.size() + 1, str.c_str());
	//							GlobalUnlock(hClipboardData);
	//							SetClipboardData(CF_TEXT, hClipboardData);
	//							CloseClipboard();
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
}