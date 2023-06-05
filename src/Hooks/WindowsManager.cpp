#include "Hooks/WindowsManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "Helpers/DebugHelper.h"
#include "Hooks/InputManager.h"
#include "InGameIme.h"
#include "Utils.h"

namespace Hooks
{
	HRESULT __fastcall WindowsManager::WindowProc_Hook::hooked(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		InGameIME* pInGameIme = InGameIME::GetSingleton();
		RE::ControlMap* pControlMap = RE::ControlMap::GetSingleton();
		auto main = RE::Main::GetSingleton();
		Cicero* pCicero = Cicero::GetSingleton();

		switch (uMsg) {
		case WM_ACTIVATE:
			if (wParam == WA_ACTIVE) {
				DH_DEBUG("[WinProc WM_ACTIVATE] Window Activated");
				pInGameIme->inputContent = std::wstring();
			}
			break;

		case WM_INPUTLANGCHANGE:
			DH_DEBUG("[WinProc WM_INPUTLANGCHANGE]");
			break;

		case WM_IME_NOTIFY:
			pInGameIme->ProcessImeNotify(hWnd, wParam, lParam);
			return 0;

		case WM_IME_STARTCOMPOSITION:
			DH_DEBUG("[WinProc WM_IME_STARTCOMPOSITION]");
			if (pControlMap->textEntryCount) {
				// Focusing on a input area
				InterlockedExchange(&pInGameIme->enableState, 1);
				InterlockedExchange(&pInGameIme->disableKeyState, 1);

				if (!pCicero->ciceroState) {
					DEBUG("[WinProc$WM_IME_STARTCOMPOSITION] Cicero is disabled");
				}
			}
			return S_OK;

		case WM_IME_COMPOSITION:
			DH_DEBUG("[WinProc WM_IME_COMPOSITION]");
			if (pControlMap->textEntryCount) {
				if (lParam & GCS_CURSORPOS) {}
				if (lParam & CS_INSERTCHAR) {}
				if (lParam & GCS_COMPSTR && !pCicero->ciceroState)  // Only handle this message when TSF is disabled
					Utils::GetInputString(hWnd);
				if (lParam & GCS_RESULTSTR)
					Utils::GetResultString(hWnd);
			}
			return S_OK;

		case WM_IME_ENDCOMPOSITION:
			InterlockedExchange(&pInGameIme->enableState, 0);

			if (!pCicero->ciceroState) {
				pInGameIme->ime_critical_section.Enter();
				DH_DEBUG("[WinProc WM_IME_ENDCOMPOSITION] Clearing InputContent and CandidateList");
				pInGameIme->candidateList.clear();
				pInGameIme->inputContent.clear();
				pInGameIme->ime_critical_section.Leave();
			}
			if (pControlMap->textEntryCount) {
				auto f = [=](UINT32 time) -> bool {
					std::this_thread::sleep_for(std::chrono::milliseconds(time));
					if (!InterlockedCompareExchange(&pInGameIme->enableState, pInGameIme->enableState, 2)) {
						InterlockedExchange(&pInGameIme->disableKeyState, 0);
					}
					return true;
				};
				std::packaged_task<bool(UINT32)> task(f);
				std::future<bool> fut = task.get_future();
				std::thread task_td(std::move(task), 150);
				task_td.detach();
			}
			break;

		case WM_IME_SETSTATE:
			if (lParam == WIME_STATE_ENABLE) {
				DH_DEBUG("[WinPrc WM_IME_SETSTATE] Enable IME");
				// Restore the default input method context of the window.
				ImmAssociateContextEx(hWnd, NULL, IACE_DEFAULT);
			} else {
				DH_DEBUG("[WinPrc WM_IME_SETSTATE] Disable");
				ImmAssociateContextEx(hWnd, NULL, NULL);
			}
			break;

		case WM_KEYDOWN:
			// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
			if (wParam == VK_INSERT) {
				DH_DEBUG("[WinProc WM_KEYDOWN] Insert key down, switching IME", uMsg, wParam);
				DWORD threadId = GetWindowThreadProcessId(hWnd, NULL);
				HKL inputContext = GetKeyboardLayout(threadId);
				ActivateKeyboardLayout((HKL)((ULONG_PTR)inputContext + 1), 0);
			}
			break;

		case WM_CHAR:
			DH_DEBUG("[WinProc WM_CHAR] Msg: {} Param: {}", uMsg, wParam);
			if (pControlMap->textEntryCount) {
				Utils::SendUnicodeMessage(wParam);
			}
			return S_OK;

		case WM_IME_SETCONTEXT:
			return DefWindowProc(hWnd, WM_IME_SETCONTEXT, wParam, NULL);
		}
		return oldFunc(hWnd, uMsg, wParam, lParam);
	}

	void WindowsManager::Install()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DEBUG("Hooking WindowProc");
		Utils::DetourAttach<WindowProc_Hook>(REL::ID(36649));

		DetourTransactionCommit();
	}
}