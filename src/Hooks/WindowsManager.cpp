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

		case WM_IME_NOTIFY:
			switch (wParam) {
			case IMN_OPENCANDIDATE:
			case IMN_SETCANDIDATEPOS:
			case IMN_CHANGECANDIDATE:
				int token = rand();

				DH_DEBUG("[WinProc IME_NOTIFY#{}] WPARAM: {:X}", token, wParam);
				pInGameIme->bEnabled = IME_UI_ENABLED;
				if (pControlMap->textEntryCount) {
					Utils::UpdateCandidateList(hWnd);
				}
			}
			return S_OK;

		case WM_INPUTLANGCHANGE:
			Utils::UpdateInputMethodName((HKL)lParam);
			break;

		case WM_IME_STARTCOMPOSITION:
			DH_DEBUG("[WinProc WM_IME_STARTCOMPOSITION]");
			if (pControlMap->textEntryCount) {  // Focusing on a input area
				pInGameIme->bEnabled = IME_UI_ENABLED;
				pInGameIme->bDisableSpecialKey = TRUE;
			}
			return S_OK;

		case WM_IME_COMPOSITION:
			DH_DEBUG("[WinProc WM_IME_COMPOSITION]");
			if (pControlMap->textEntryCount) {
				if (lParam & GCS_COMPSTR)
					Utils::UpdateInputContent(hWnd);
				if (lParam & GCS_RESULTSTR)
					Utils::GetResultString(hWnd);
			}
			return S_OK;

		case WM_IME_ENDCOMPOSITION:
			DH_DEBUG("[WinProc WM_IME_ENDCOMPOSITION] Clearing candidate list and input content");
			InterlockedExchange(&pInGameIme->bEnabled, FALSE);

			pInGameIme->imeCriticalSection.Enter();
			pInGameIme->candidateList.clear();
			pInGameIme->inputContent.clear();
			pInGameIme->imeCriticalSection.Leave();

			if (pControlMap->textEntryCount) {
				auto f = [=](UINT32 time) -> bool {
					std::this_thread::sleep_for(std::chrono::milliseconds(time));
					if (!pInGameIme->bEnabled) {
						InterlockedExchange(&pInGameIme->bDisableSpecialKey, FALSE);
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
				DH_DEBUG("[WinProc WM_IME_SETSTATE] Enable IME");
				ImmAssociateContextEx(hWnd, NULL, IACE_DEFAULT);
			} else {
				DH_DEBUG("[WinProc WM_IME_SETSTATE] Disable IME");
				ImmAssociateContextEx(hWnd, NULL, NULL);
			}
			break;

		case WM_CHAR:
			DH_DEBUG("[WinProc WM_CHAR] Param: {}", wParam);
			if (pControlMap->textEntryCount) {
				if (wParam == VK_SPACE && GetKeyState(VK_LWIN) < 0) {
					ActivateKeyboardLayout((HKL)HKL_NEXT, KLF_SETFORPROCESS);
					return S_OK;
				}
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

		DH_DEBUG("Hooking WindowProc");
		Utils::DetourAttach<WindowProc_Hook>(REL::ID(36649));

		DetourTransactionCommit();
	}
}