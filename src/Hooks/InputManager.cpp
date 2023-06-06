#include "Hooks/InputManager.h"

#include "detours/Detours.h"

#include "Helpers/DebugHelper.h"
#include "Hooks/WindowsManager.h"
#include "InputPanel.h"
#include "RE/CustomRE.h"
#include "RE/Offset.h"
#include "Utils.h"

namespace Hooks
{
	void InputManager::Install()
	{
		char name[MAX_PATH] = "\0";
		GetModuleFileNameA(GetModuleHandle(NULL), name, MAX_PATH);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DH_INFO("Installing ControlMap::AllowTextInput Hook");
		Utils::DetourAttach<ControlMap_AllowTextInput_Hook>(RE::Offset::ControlMap::AllowTextInput);

		DetourTransactionCommit();

		DH_INFO("Installing UIMessageQueue::AddMessage Hook at 82182+0x7D");
		Utils::WriteCall<UIMessageQueue_AddMessage_Hook>(REL::RelocationID(0, 82182).address() + REL::Relocate(0, 0x7D));
		GetModuleHandle(NULL);

		DH_INFO("Installing dinput8.dll DirectInput8Create Hook");
		auto hook = DKUtil::Hook::AddIATHook(name, "dinput8.dll", "DirectInput8Create", FUNC_INFO(DLL_DInput8_DirectInput8Create_Hook::hooked));
		DLL_DInput8_DirectInput8Create_Hook::oldFunc = REL::Relocation<decltype(DLL_DInput8_DirectInput8Create_Hook::hooked)>(hook->OldAddress);
		hook->Enable();
	}

	HRESULT WINAPI InputManager::DLL_DInput8_DirectInput8Create_Hook::hooked(HINSTANCE a_hInstance, DWORD a_dwVersion, REFIID a_id, void* a_pvOut, IUnknown* a_pOuter)
	{
		IDirectInput8A* pDinput8 = nullptr;
		HRESULT hr = oldFunc(a_hInstance, a_dwVersion, a_id, &pDinput8, a_pOuter);
		if (hr != S_OK)
			return hr;
		*((IDirectInput8A**)a_pvOut) = new SCIDirectInput(pDinput8);

		DH_INFO("Hooked DirectInput8Create@dinput8.dll");

		return S_OK;
	}

	void InputManager::ProcessAllowTextInput(bool a_bOpenInput)
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

	void InputManager::UIMessageQueue_AddMessage_Hook::hooked(RE::UIMessageQueue* a_pThis, const RE::BSFixedString& a_menuName, RE::UI_MESSAGE_TYPE a_type, RE::IUIMessageData* a_data)
	{
		using namespace RE;
		using KeyCode = GFxKey::Code;
		if (a_type == RE::UI_MESSAGE_TYPE::kScaleformEvent && a_data != nullptr) {
			BSUIScaleformData* pData = reinterpret_cast<BSUIScaleformData*>(a_data);
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
							DH_DEBUG("[UIMessageQueue::AddMessage] Ignored key {}", ignored_key);
							Utils::HeapFree(a_data);
							return;
						}
					}
				}
			} else if (event->type == GFxEvent::EventType::kCharEvent) {
				GFxCharEvent* pCharEvent = static_cast<GFxCharEvent*>(event);
				DH_DEBUG("[UIMessageQueue::AddMessage] Char Event, Code {}", (char)pCharEvent->wcharCode);
				Utils::HeapFree(a_data);
				return;
			}
		}
		oldFunc(a_pThis, a_menuName, a_type, a_data);
	}

	HRESULT WINAPI SCIDirectInputDevice::SetCooperativeLevel(HWND a1, DWORD a2) noexcept
	{
		if (m_eDeviceType == kKeyboard) {
			return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);
		} else {
			return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
		}
	}

	ULONG WINAPI SCIDirectInputDevice::AddRef() noexcept
	{
		m_ulRefs++;
		return m_ulRefs;
	}

	ULONG WINAPI SCIDirectInputDevice::Release() noexcept
	{
		m_ulRefs--;
		if (!m_ulRefs) {
			m_pOriginDevice->Release();
			delete this;
			return 0;
		}
		return m_ulRefs;
	}

	ULONG WINAPI SCIDirectInput::AddRef(void) noexcept
	{
		m_ulRefs++;
		return m_ulRefs;
	}

	ULONG WINAPI SCIDirectInput::Release(void) noexcept
	{
		m_ulRefs--;
		if (!m_ulRefs) {
			m_pOrigin->Release();
			delete this;
			return 0;
		}
		return m_ulRefs;
	}

	HRESULT WINAPI SCIDirectInput::CreateDevice(REFGUID a_gidType, LPDIRECTINPUTDEVICE8A* a_pDevice, LPUNKNOWN unused) noexcept
	{
		if (a_gidType != GUID_SysKeyboard && a_gidType != GUID_SysMouse) {
			return m_pOrigin->CreateDevice(a_gidType, a_pDevice, unused);
		} else {
			IDirectInputDevice8A* device;
			HRESULT hr = m_pOrigin->CreateDevice(a_gidType, &device, unused);
			if (hr != S_OK)
				return hr;
			*a_pDevice = new SCIDirectInputDevice(device, a_gidType == GUID_SysKeyboard ? SCIDirectInputDevice::kKeyboard : SCIDirectInputDevice::kMouse);
			return hr;
		}
	}
}