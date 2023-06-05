#include "Hooks/InputManager.h"

#include "Hooks/WindowsManager.h"

#include "Helpers/DebugHelper.h"
#include "RE/CustomRE.h"
#include "RE/Offset.h"
#include "detours/Detours.h"

#include "InGameIme.h"
#include "Utils.h"

namespace Hooks
{
	HRESULT WINAPI InputManager::DLL_DInput8_DirectInput8Create_Hook::hooked(HINSTANCE instance, DWORD version, REFIID iid, void* out, IUnknown* outer)
	{
		IDirectInput8A* pDinput8 = nullptr;
		HRESULT hr = oldFunc(instance, version, iid, &pDinput8, outer);
		if (hr != S_OK)
			return hr;
		*((IDirectInput8A**)out) = new SCIDirectInput(pDinput8);

		DH_INFO("Hooked DirectInput8Create@dinput8.dll");

		return S_OK;
	}

	void InputManager::Install()
	{
		char name[MAX_PATH] = "\0";
		GetModuleFileNameA(GetModuleHandle(NULL), name, MAX_PATH);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		INFO("Installing ControlMap::AllowTextInput Hook");
		Utils::DetourAttach<ControlMap_AllowTextInput_Hook>(RE::Offset::ControlMap::AllowTextInput);

		DetourTransactionCommit();

		INFO("Installing UIMessageQueue::AddMessage Hook at 82182+0x7D");
		Utils::WriteCall<UIMessageQueue_AddMessage_Hook>(REL::RelocationID(0, 82182).address() + REL::Relocate(0, 0x7D));
		GetModuleHandle(NULL);

		INFO("Installing dinput8.dll DirectInput8Create Hook");
		auto hook = DKUtil::Hook::AddIATHook(name, "dinput8.dll", "DirectInput8Create", FUNC_INFO(DLL_DInput8_DirectInput8Create_Hook::hooked));
		DLL_DInput8_DirectInput8Create_Hook::oldFunc = REL::Relocation<decltype(DLL_DInput8_DirectInput8Create_Hook::hooked)>(hook->OldAddress);
		hook->Enable();
	}

	void InputManager::ProcessAllowTextInput(bool increase)
	{
		DEBUG("ProcessAllowTextInput ({} InputBox)", increase ? "Open" : "Close");

		auto pControlMap = RE::ControlMap::GetSingleton();
		auto pInputManager = InputManager::GetSingleton();
		auto game_main = RE::Main::GetSingleton();
		auto pInputMenu = InGameIME::GetSingleton();

		std::uint8_t currentCount = pControlMap->textEntryCount;

		if (!increase && currentCount == 1) {
			// 退出最后一个输入窗口
			DEBUG("Post WM_IME_SETSTATE DISABLE to {}", (uintptr_t)pInputMenu->hwnd);
			// 禁用输入法
			PostMessage(pInputMenu->hwnd, WM_IME_SETSTATE, NULL, 0);

			// 清除候选词列表
			pInputMenu->imeCriticalSection.Enter();
			DH_DEBUG("(ProcessAllowTextInput) Clearing CandidateList")
			pInputMenu->candidateList.clear();
			pInputMenu->inputContent.clear();
			pInputMenu->bEnabled = IME_UI_DISABLED;
			pInputMenu->imeCriticalSection.Leave();
		} else if (increase && currentCount == 0) {
			// 打开第一个输入窗口
			DEBUG("Post WM_IME_SETSTATE ENABLE to {}", (uintptr_t)pInputMenu->hwnd);
			// 启用输入法
			PostMessage(pInputMenu->hwnd, WM_IME_SETSTATE, NULL, 1);
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
				// 按键事件
				InGameIME* pInGameIme = InGameIME::GetSingleton();
				GFxKeyEvent* key = static_cast<GFxKeyEvent*>(event);

				if (InterlockedCompareExchange(&pInGameIme->bDisableSpecialKey, pInGameIme->bDisableSpecialKey, 2)) {
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
				// 字符事件
				GFxCharEvent* charEvent = static_cast<GFxCharEvent*>(event);
				DH_DEBUG("[UIMessageQueue::AddMessage] Char Event, Code {}", (char)charEvent->wcharCode);
				Utils::HeapFree(a_data);
				return;
			}
		}
		oldFunc(a_pThis, a_menuName, a_type, a_data);
	}

	HRESULT WINAPI SCIDirectInputDevice::SetCooperativeLevel(HWND a1, DWORD a2) noexcept
	{
		if (m_deviceType == kKeyboard) {
			return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);
		} else {
			return m_pOriginDevice->SetCooperativeLevel(a1, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
		}
	}

	ULONG WINAPI SCIDirectInputDevice::AddRef() noexcept
	{
		m_refs++;
		return m_refs;
	}

	ULONG WINAPI SCIDirectInputDevice::Release() noexcept
	{
		m_refs--;
		if (!m_refs) {
			m_pOriginDevice->Release();
			delete this;
			return 0;
		}
		return m_refs;
	}

	ULONG WINAPI SCIDirectInput::AddRef(void) noexcept
	{
		m_refs++;
		return m_refs;
	}

	ULONG WINAPI SCIDirectInput::Release(void) noexcept
	{
		m_refs--;
		if (!m_refs) {
			m_pOrigin->Release();
			delete this;
			return 0;
		}
		return m_refs;
	}

	HRESULT WINAPI SCIDirectInput::CreateDevice(REFGUID a_type, LPDIRECTINPUTDEVICE8A* a_pDevice, LPUNKNOWN unused) noexcept
	{
		if (a_type != GUID_SysKeyboard && a_type != GUID_SysMouse) {
			return m_pOrigin->CreateDevice(a_type, a_pDevice, unused);
		} else {
			IDirectInputDevice8A* device;
			HRESULT hr = m_pOrigin->CreateDevice(a_type, &device, unused);
			if (hr != S_OK)
				return hr;
			*a_pDevice = new SCIDirectInputDevice(device, a_type == GUID_SysKeyboard ? SCIDirectInputDevice::kKeyboard : SCIDirectInputDevice::kMouse);
			return hr;
		}
	}
}