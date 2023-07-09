#include "Hooks/InputManager.h"

#include "detours/Detours.h"

#include "Config.h"
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
		DISABLE_IME(pIMEPanel->hWindow);

		pIMEPanel->csImeInformation.Enter();
		DEBUG("(ProcessAllowTextInput) Clearing CandidateList")
		pIMEPanel->vwsCandidateList.clear();
		pIMEPanel->wstrComposition.clear();
		pIMEPanel->m_bEnabled.store(false);
		pIMEPanel->csImeInformation.Leave();
	} else if (a_bOpenInput && currentCount == 0) {
		DEBUG("Post WM_IME_SETSTATE ENABLE to {}", (uintptr_t)pIMEPanel->hWindow);
		// Enable IME
		ENABLE_IME(pIMEPanel->hWindow);
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

			if (pIMEPanel->m_bDisableSpecialKey.load()) {
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

using namespace DKUtil::Alias;
constexpr OpCode NOP[]{ 0x90, 0x90 };

void Hooks::InputManager::Install()
{
	constexpr Patch NopPatch = { std::addressof(NOP), sizeof(NOP) };
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	INFO("Installing ControlMap::AllowTextInput Hook");
	Hooks::InputManager::Hook_ControlMap_AllowTextInput::Install();

	DetourTransactionCommit();

	INFO("Installing UnknownFunction_UIMessageQueue::AddMessage() Hook");
	Hooks::InputManager::Hook_UIMessageQueue_AddMessage::Install();

	INFO("Installing dinput8.dll DirectInput8Create Hook");
	DLL_DInput8_DirectInput8Create_Hook::oldFunc = SKSE::PatchIAT(DLL_DInput8_DirectInput8Create_Hook::hooked, "dinput8.dll", "DirectInput8Create");

	constexpr int offsetRM1SE = 0x8B13A5 - 0x8B1360;  // 0x45
	constexpr int offsetRM1AE = 0x8F3803 - 0x8F37C0;  // 0x43
	auto offsetRM1 = REL::Relocate(offsetRM1SE, offsetRM1AE);
	dku::Hook::AddASMPatch(RELOCATION_ID(51488, 52350).address(), { offsetRM1, offsetRM1 + 0x2 }, &NopPatch)->Enable();

	constexpr int offsetEM1SE = 0x863199 - 0x862C80;  // 0x519
	constexpr int offsetEM1AE = 0x8A2CFA - 0x8A27E0;  // 0x51A
	auto offsetEM1 = REL::Relocate(offsetEM1SE, offsetEM1AE);
	dku::Hook::AddASMPatch(RELOCATION_ID(50274, 51200).address(), { offsetEM1, offsetEM1 + 0x2 }, &NopPatch)->Enable();

	constexpr int offsetRM2SE = 0x86DED1 - 0x86DE80;  // 0x51
	constexpr int offsetRM2AE = 0x8AE699 - 0x8AE650;  // 0x49
	auto offsetRM2 = REL::Relocate(offsetRM2SE, offsetRM2AE);
	dku::Hook::AddASMPatch(RELOCATION_ID(50473, 51366).address(), { offsetRM2, offsetRM2 + 0x2 }, &NopPatch)->Enable();

	constexpr int offsetRM3SE = 0x8B0733 - 0x8B0200;  // 0x533
	constexpr int offsetRM3AE = 0x8F2979 - 0x8F2510;  // 0x469
	auto offsetRM3 = REL::Relocate(offsetRM3SE, offsetRM3AE);
	dku::Hook::AddASMPatch(RELOCATION_ID(51483, 52345).address(), { offsetRM3, offsetRM3 + 0x2 }, &NopPatch)->Enable();
}

namespace Hooks
{

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