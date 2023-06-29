#include "Hooks/InputManager.h"

#include "detours/Detours.h"

#include "Config.h"
#include "Helpers/DebugHelper.h"
#include "Hooks/WindowsManager.h"
#include "InputPanel.h"
#include "RE/CustomRE.h"
#include "RE/Offset.h"
#include "Utils.h"

#pragma region Hooks
void Hooks::InputManager::ProcessAllowTextInput(bool a_bOpenInput)
{
	DEBUG("ProcessAllowTextInput ({} InputBox)", a_bOpenInput ? "Open" : "Close");

	auto pControlMap = RE::ControlMap::GetSingleton();
	auto pInputManager = InputManager::GetSingleton();
	auto pMain = RE::Main::GetSingleton();
	auto pIMEPanel = IMEPanel::GetSingleton();

	std::uint8_t currentCount = pControlMap->textEntryCount;

	if (!a_bOpenInput && currentCount == 1) {
		DEBUG("Post WM_IME_SETSTATE DISABLE to {}", (uintptr_t)pIMEPanel->hWindow);
		// Disable IME
		PostMessage(pIMEPanel->hWindow, WM_IME_SETSTATE, NULL, 0);

		pIMEPanel->csImeInformation.Enter();
		DH_DEBUG("(ProcessAllowTextInput) Clearing CandidateList")
		pIMEPanel->vwsCandidateList.clear();
		pIMEPanel->wstrComposition.clear();
		pIMEPanel->bEnabled = IME_UI_DISABLED;
		pIMEPanel->csImeInformation.Leave();
	} else if (a_bOpenInput && currentCount == 0) {
		DEBUG("Post WM_IME_SETSTATE ENABLE to {}", (uintptr_t)pIMEPanel->hWindow);
		// Enable IME
		PostMessage(pIMEPanel->hWindow, WM_IME_SETSTATE, NULL, 1);
	}
}

void __fastcall Hooks::InputManager::Hook_ControlMap_AllowTextInput::hooked(RE::ControlMap* a_pThis, bool a_bAddRef)
{
	auto pInputManager = InputManager::GetSingleton();

	if (pInputManager) {
		pInputManager->ProcessAllowTextInput(a_bAddRef);
	}
	oldFunc(a_pThis, a_bAddRef);
}

void Hooks::InputManager::Hook_UIMessageQueue_AddMessage::hooked(RE::UIMessageQueue* a_pThis, const RE::BSFixedString& a_sMenuName, RE::UI_MESSAGE_TYPE a_cMessageType, RE::IUIMessageData* a_pData)
{
	using namespace RE;
	using KeyCode = GFxKey::Code;
	if (a_cMessageType == RE::UI_MESSAGE_TYPE::kScaleformEvent && a_pData != nullptr) {
		BSUIScaleformData* pData = reinterpret_cast<BSUIScaleformData*>(a_pData);
		GFxEvent* event = pData->scaleformEvent;

		if (event->type == GFxEvent::EventType::kKeyDown && ControlMap::GetSingleton()->textEntryCount) {
			IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
			GFxKeyEvent* key = static_cast<GFxKeyEvent*>(event);

			if (InterlockedCompareExchange(&pIMEPanel->bDisableSpecialKey, pIMEPanel->bDisableSpecialKey, 2)) {
				static std::vector<uint32_t> ignored_keys{
					KeyCode::kReturn,
					KeyCode::kBackspace,
					KeyCode::kKP_Enter,
					KeyCode::kEscape,
					KeyCode::kLeft,
					KeyCode::kRight,
					KeyCode::kUp,
					KeyCode::kDown,
				};
				for (auto& ignored_key : ignored_keys) {
					if (ignored_key == key->keyCode) {
						DEBUG("[UIMessageQueue::AddMessage] Ignored key {}", ignored_key);
						Utils::HeapFree(a_pData);
						return;
					}
				}
			}
		} else if (event->type == GFxEvent::EventType::kCharEvent) {
			GFxCharEvent* pCharEvent = static_cast<GFxCharEvent*>(event);
			DEBUG("[UIMessageQueue::AddMessage] Char Event, Code {}", (char)pCharEvent->wcharCode);
			Utils::HeapFree(a_pData);
			return;
		}
	}
	oldFunc(a_pThis, a_sMenuName, a_cMessageType, a_pData);
}
#pragma endregion

namespace Hooks
{
	void InputManager::Install()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		INFO("Installing ControlMap::AllowTextInput Hook");
		Utils::Hook::DetourAttach<Hook_ControlMap_AllowTextInput>(RE::Offset::ControlMap::AllowTextInput.address());

		DetourTransactionCommit();

		INFO("Installing UnknownFunction_UIMessageQueue::AddMessage() Hook");
		// SE :$85AA30 :$EC3ED0(80079), AE :$f1c650
		Utils::WriteCall<Hooks::InputManager::Hook_UIMessageQueue_AddMessage>(RELOCATION_ID(80079, 82182).address() + REL::Relocate(0x9C, 0x7D));
		GetModuleHandle(NULL);

		INFO("Installing dinput8.dll DirectInput8Create Hook");
		DLL_DInput8_DirectInput8Create_Hook::oldFunc = SKSE::PatchIAT(DLL_DInput8_DirectInput8Create_Hook::hooked, "dinput8.dll", "DirectInput8Create");
	}

	HRESULT WINAPI InputManager::DLL_DInput8_DirectInput8Create_Hook::hooked(HINSTANCE a_hInstance, DWORD a_dwVersion, REFIID a_id, void* a_pvOut, IUnknown* a_pOuter)
	{
		INFO("Replacing IDirectInput8A");
		IDirectInput8A* pDinput8 = nullptr;
		HRESULT hr = oldFunc(a_hInstance, a_dwVersion, a_id, &pDinput8, a_pOuter);
		if (hr != S_OK)
			return hr;
		*((IDirectInput8A**)a_pvOut) = new SIMDirectInput(pDinput8);
		return S_OK;
	}

	HRESULT WINAPI SIMDirectInputDevice::SetCooperativeLevel(HWND a1, DWORD a2) noexcept
	{
		INFO("Setting CooperativeLevel");
		if (m_eDeviceType == kKeyboard) {
			if (Configs::bFeatureUnlockWinKey) {
				return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			} else {
				return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);
			}
		} else {
			return m_pOriginDevice->SetCooperativeLevel(a1, a2);
		}
	}

	ULONG WINAPI SIMDirectInputDevice::AddRef() noexcept
	{
		m_ulRefs++;
		return m_ulRefs;
	}

	ULONG WINAPI SIMDirectInputDevice::Release() noexcept
	{
		m_ulRefs--;
		if (!m_ulRefs) {
			m_pOriginDevice->Release();
			delete this;
			return 0;
		}
		return m_ulRefs;
	}

	ULONG WINAPI SIMDirectInput::AddRef(void) noexcept
	{
		m_ulRefs++;
		return m_ulRefs;
	}

	ULONG WINAPI SIMDirectInput::Release(void) noexcept
	{
		m_ulRefs--;
		if (!m_ulRefs) {
			m_pOrigin->Release();
			delete this;
			return 0;
		}
		return m_ulRefs;
	}

	HRESULT WINAPI SIMDirectInput::CreateDevice(REFGUID a_gidType, LPDIRECTINPUTDEVICE8A* a_pDevice, LPUNKNOWN unused) noexcept
	{
		if (a_gidType == GUID_SysKeyboard) {
			IDirectInputDevice8A* device;
			// Creates the old device
			HRESULT hr = m_pOrigin->CreateDevice(a_gidType, &device, unused);
			if (hr != S_OK)
				return hr;
			// returns the packed device
			*a_pDevice = new SIMDirectInputDevice(device, SIMDirectInputDevice::kKeyboard);
			return hr;
		} else {
			return m_pOrigin->CreateDevice(a_gidType, a_pDevice, unused);
		}
	}
}