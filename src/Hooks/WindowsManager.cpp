#include "Hooks/WindowsManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "Helpers/DebugHelper.h"
#include "Hooks/InputManager.h"
#include "InputPanel.h"
#include "Utils.h"

namespace Hooks
{
	HRESULT __fastcall WindowsManager::WindowProc_Hook::hooked(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
		RE::ControlMap* pControlMap = RE::ControlMap::GetSingleton();
		auto main = RE::Main::GetSingleton();
		Cicero* pCicero = Cicero::GetSingleton();

		switch (uMsg) {
		case WM_ACTIVATE:
			if (wParam == WA_ACTIVE) {
				DH_DEBUG("[WinProc WM_ACTIVATE] Window Activated");
				pIMEPanel->wstrComposition = std::wstring();
			}
			break;

		case WM_IME_NOTIFY:
			switch (wParam) {
			case IMN_OPENCANDIDATE:
			case IMN_SETCANDIDATEPOS:
			case IMN_CHANGECANDIDATE:
				int token = rand();

				DH_DEBUG("[WinProc IME_NOTIFY#{}] WPARAM: {:X}", token, wParam);
				pIMEPanel->bEnabled = IME_UI_ENABLED;
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
				pIMEPanel->bEnabled = IME_UI_ENABLED;
				pIMEPanel->bDisableSpecialKey = TRUE;
			}
			return S_OK;

		case WM_IME_COMPOSITION:
			if (pControlMap->textEntryCount) {
				// According ImeUI.cpp in DXUT, GCS_RESULTSTR Must
				if (lParam & GCS_RESULTSTR) {
					DH_DEBUG("[WinProc WM_IME_COMPOSITION] Updating Result String");
					Utils::GetResultString(hWnd);
				}
				if (lParam & GCS_COMPSTR) {
					DH_DEBUG("[WinProc WM_IME_COMPOSITION] Updating Composition String");
					Utils::UpdateInputContent(hWnd);
				}
			}
			return S_OK;

		case WM_IME_ENDCOMPOSITION:
			DH_DEBUG("[WinProc WM_IME_ENDCOMPOSITION] Clearing candidate list and input content");
			InterlockedExchange(&pIMEPanel->bEnabled, FALSE);

			pIMEPanel->csImeInformation.Enter();
			pIMEPanel->vwsCandidateList.clear();
			pIMEPanel->wstrComposition.clear();
			pIMEPanel->csImeInformation.Leave();

			if (pControlMap->textEntryCount) {
				auto f = [=](UINT32 time) -> bool {
					std::this_thread::sleep_for(std::chrono::milliseconds(time));
					if (!pIMEPanel->bEnabled) {
						InterlockedExchange(&pIMEPanel->bDisableSpecialKey, FALSE);
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

		DH_INFO("Installing WindowProc Hook");
		Utils::DetourAttach<WindowProc_Hook>(REL::ID(36649));

		DetourTransactionCommit();
	}
}