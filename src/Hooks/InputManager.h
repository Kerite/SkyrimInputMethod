#pragma once

#include <dinput.h>

#include "Helpers/DebugHelper.h"
#include "RE/Offset.h"

#define WIME_STATE_DISABLE 0
#define WIME_STATE_ENABLE 1

// Hook 的位置：ID 82182 偏移 0x7D（0xF1C6CD-0xF1C650）调用AddMessage的位置

namespace Hooks
{
	typedef void (*AllowTextInputFunc)(RE::ControlMap* a_controlMap, bool a_allow);

	class InputManager final : public Singleton<InputManager>
	{
	public:
		void Install();

		void ProcessAllowTextInput(bool a_increase);

	private:
		HOOK_DEF_THIS(ControlMap_AllowTextInput, void, RE::ControlMap, bool);
		HOOK_DEF_THIS(UIMessageQueue_AddMessage, void, RE::UIMessageQueue, const RE::BSFixedString&, RE::UI_MESSAGE_TYPE, RE::IUIMessageData*)

		struct DLL_DInput8_DirectInput8Create_Hook
		{
			static HRESULT WINAPI hooked(HINSTANCE a_hInstance, DWORD a_dwVersion, REFIID a_id, void* a_pvOut, IUnknown* a_pOuter);
			static inline REL::Relocation<decltype(hooked)> oldFunc;
		};
	};

	// Used for making the input device non-exclusive
	class SIMDirectInputDevice : public IDirectInputDevice8A
	{
	public:
		enum DeviceType
		{
			kKeyboard,
			kMouse  // Not used
		};

		SIMDirectInputDevice(IDirectInputDevice8A* a_pDevice, DeviceType a_deviceType) :
			m_pOriginDevice(a_pDevice), m_eDeviceType(a_deviceType), m_ulRefs(1) {}

		HRESULT WINAPI SetCooperativeLevel(HWND a1, DWORD a2) noexcept;
		ULONG WINAPI AddRef() noexcept;
		ULONG WINAPI Release() noexcept;

		HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) noexcept { return m_pOriginDevice->QueryInterface(riid, ppvObj); }
		HRESULT WINAPI GetCapabilities(LPDIDEVCAPS a_pDidevCaps) noexcept { return m_pOriginDevice->GetCapabilities(a_pDidevCaps); }
		HRESULT WINAPI EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA a1, LPVOID a2, DWORD a3) noexcept { return m_pOriginDevice->EnumObjects(a1, a2, a3); }
		HRESULT WINAPI GetProperty(REFGUID a1, LPDIPROPHEADER a2) noexcept { return m_pOriginDevice->GetProperty(a1, a2); }
		HRESULT WINAPI SetProperty(REFGUID a1, LPCDIPROPHEADER a2) noexcept { return m_pOriginDevice->SetProperty(a1, a2); }
		HRESULT WINAPI Acquire() noexcept { return m_pOriginDevice->Acquire(); }
		HRESULT WINAPI Unacquire() noexcept { return m_pOriginDevice->Unacquire(); }
		HRESULT WINAPI GetDeviceState(DWORD a1, LPVOID a2) noexcept { return m_pOriginDevice->GetDeviceState(a1, a2); }
		HRESULT WINAPI GetDeviceData(DWORD a1, LPDIDEVICEOBJECTDATA a2, LPDWORD a3, DWORD a4) noexcept { return m_pOriginDevice->GetDeviceData(a1, a2, a3, a4); }
		HRESULT WINAPI SetDataFormat(LPCDIDATAFORMAT a1) noexcept { return m_pOriginDevice->SetDataFormat(a1); }
		HRESULT WINAPI SetEventNotification(HANDLE a) noexcept { return m_pOriginDevice->SetEventNotification(a); }
		HRESULT WINAPI GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA a1, DWORD a2, DWORD a3) noexcept { return m_pOriginDevice->GetObjectInfo(a1, a2, a3); }
		HRESULT WINAPI GetDeviceInfo(LPDIDEVICEINSTANCEA a1) noexcept { return m_pOriginDevice->GetDeviceInfo(a1); }
		HRESULT WINAPI RunControlPanel(HWND a1, DWORD a2) noexcept { return m_pOriginDevice->RunControlPanel(a1, a2); }
		HRESULT WINAPI Initialize(HINSTANCE a1, DWORD a2, REFGUID a3) noexcept { return m_pOriginDevice->Initialize(a1, a2, a3); }
		HRESULT WINAPI CreateEffect(REFGUID a1, LPCDIEFFECT a2, LPDIRECTINPUTEFFECT* a3, LPUNKNOWN a4) noexcept { return m_pOriginDevice->CreateEffect(a1, a2, a3, a4); }
		HRESULT WINAPI EnumEffects(LPDIENUMEFFECTSCALLBACKA a1, LPVOID a2, DWORD a3) noexcept { return m_pOriginDevice->EnumEffects(a1, a2, a3); }
		HRESULT WINAPI GetEffectInfo(LPDIEFFECTINFOA a1, REFGUID a2) noexcept { return m_pOriginDevice->GetEffectInfo(a1, a2); }
		HRESULT WINAPI GetForceFeedbackState(LPDWORD a1) noexcept { return m_pOriginDevice->GetForceFeedbackState(a1); }
		HRESULT WINAPI SendForceFeedbackCommand(DWORD a1) noexcept { return m_pOriginDevice->SendForceFeedbackCommand(a1); }
		HRESULT WINAPI EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a1, LPVOID a2, DWORD a3) noexcept { return m_pOriginDevice->EnumCreatedEffectObjects(a1, a2, a3); }
		HRESULT WINAPI Escape(LPDIEFFESCAPE a1) noexcept { return m_pOriginDevice->Escape(a1); }
		HRESULT WINAPI Poll() noexcept { return m_pOriginDevice->Poll(); }
		HRESULT WINAPI SendDeviceData(DWORD a1, LPCDIDEVICEOBJECTDATA a2, LPDWORD a3, DWORD a4) noexcept { return m_pOriginDevice->SendDeviceData(a1, a2, a3, a4); }
		HRESULT WINAPI EnumEffectsInFile(LPCSTR a1, LPDIENUMEFFECTSINFILECALLBACK a2, LPVOID a3, DWORD a4) noexcept { return m_pOriginDevice->EnumEffectsInFile(a1, a2, a3, a4); }
		HRESULT WINAPI WriteEffectToFile(LPCSTR a1, DWORD a2, LPDIFILEEFFECT a3, DWORD a4) noexcept { return m_pOriginDevice->WriteEffectToFile(a1, a2, a3, a4); }
		HRESULT WINAPI BuildActionMap(LPDIACTIONFORMATA a1, LPCSTR a2, DWORD a3) noexcept { return m_pOriginDevice->BuildActionMap(a1, a2, a3); }
		HRESULT WINAPI SetActionMap(LPDIACTIONFORMATA a1, LPCSTR a2, DWORD a3) noexcept { return m_pOriginDevice->SetActionMap(a1, a2, a3); }
		HRESULT WINAPI GetImageInfo(LPDIDEVICEIMAGEINFOHEADERA a1) noexcept { return m_pOriginDevice->GetImageInfo(a1); }

	private:
		DeviceType m_eDeviceType;
		IDirectInputDevice8A* m_pOriginDevice;
		ULONG m_ulRefs;
	};

	class SIMDirectInput : public IDirectInput8A
	{
	public:
		SIMDirectInput(IDirectInput8A* a_pOrigin) :
			m_pOrigin(a_pOrigin), m_ulRefs(1) {}

		HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) noexcept { return m_pOrigin->QueryInterface(riid, ppvObj); }
		ULONG WINAPI AddRef(void) noexcept;
		ULONG WINAPI Release(void) noexcept;
		HRESULT WINAPI CreateDevice(REFGUID a_type, LPDIRECTINPUTDEVICE8A* a_pDevice, LPUNKNOWN unused) noexcept;

		// Inherited via IDirectInput8A

		HRESULT WINAPI EnumDevices(DWORD a1, LPDIENUMDEVICESCALLBACKA a2, LPVOID a3, DWORD a4) noexcept { return m_pOrigin->EnumDevices(a1, a2, a3, a4); }
		HRESULT WINAPI GetDeviceStatus(REFGUID a1) noexcept { return m_pOrigin->GetDeviceStatus(a1); }
		HRESULT WINAPI RunControlPanel(HWND a1, DWORD a2) noexcept { return m_pOrigin->RunControlPanel(a1, a2); }
		HRESULT WINAPI Initialize(HINSTANCE a1, DWORD a2) noexcept { return m_pOrigin->Initialize(a1, a2); }
		HRESULT WINAPI FindDevice(REFGUID a1, LPCSTR a2, LPGUID a3) noexcept { return m_pOrigin->FindDevice(a1, a2, a3); }
		HRESULT WINAPI EnumDevicesBySemantics(LPCSTR a1, LPDIACTIONFORMATA a2, LPDIENUMDEVICESBYSEMANTICSCBA a3, LPVOID a4, DWORD a5) noexcept { return m_pOrigin->EnumDevicesBySemantics(a1, a2, a3, a4, a5); }
		HRESULT WINAPI ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a1, LPDICONFIGUREDEVICESPARAMSA a2, DWORD a3, LPVOID a4) noexcept { return m_pOrigin->ConfigureDevices(a1, a2, a3, a4); }

	private:
		ULONG m_ulRefs;
		IDirectInput8A* m_pOrigin;
	};
}