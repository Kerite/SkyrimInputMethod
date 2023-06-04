#include "Hooks/RendererManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "InGameIme.h"
#include "Utils.h"

namespace Hooks
{
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags);
	REL::Relocation<decltype(IDXGISwapChain_Present)> old_IDXGISwapChain_Present;
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags)
	{
		InGameIME* in_game_ime = InGameIME::GetSingleton();

		in_game_ime->OnRender();

		return old_IDXGISwapChain_Present(a_this, a_syncInterval, a_flags);
	}

	HRESULT WINAPI IDXGISwapChain_ResizeTarget(IDXGISwapChain* a_pThis, const DXGI_MODE_DESC* a_pNewTargetParameters);
	REL::Relocation<decltype(IDXGISwapChain_ResizeTarget)> old_IDXGISwapChain_ResizeTarget;
	HRESULT WINAPI IDXGISwapChain_ResizeTarget(IDXGISwapChain* a_pThis, const DXGI_MODE_DESC* a_pNewTargetParameters)
	{
		HRESULT hr = old_IDXGISwapChain_ResizeTarget(a_pThis, a_pNewTargetParameters);
		if (SUCCEEDED(hr)) {
			InGameIME* pInGameIme = InGameIME::GetSingleton();
			hr = pInGameIme->OnResizeTarget();
		}
		return hr;
	}

	void RendererManager::Install()
	{
		DEBUG("Hooking Renderer_Init_InitD3D");
		Utils::WriteCall<Renderer_Init_InitD3D_Hook>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));

		INFO("Installed RendererManager");
	}

	void RendererManager::Renderer_Init_InitD3D_Hook::hooked()
	{
		oldFunc();

		auto renderer = RE::BSGraphics::Renderer::GetSingleton();
		auto in_game_ime = InGameIME::GetSingleton();

		IDXGISwapChain* swapChain = renderer->data.renderWindows[0].swapChain;
		ID3D11Device* device = renderer->data.forwarder;
		ID3D11DeviceContext* device_context = renderer->data.context;

		in_game_ime->Initialize(swapChain, device, device_context);

		auto hook_Present = dku::Hook::AddVMTHook(swapChain, 8, FUNC_INFO(IDXGISwapChain_Present));
		auto hook_ResizeTarget = dku::Hook::AddVMTHook(swapChain, 14, FUNC_INFO(IDXGISwapChain_ResizeTarget));

		old_IDXGISwapChain_Present = REL::Relocation<decltype(IDXGISwapChain_Present)>(hook_Present->OldAddress);
		old_IDXGISwapChain_ResizeTarget = REL::Relocation<decltype(IDXGISwapChain_ResizeTarget)>(hook_ResizeTarget->OldAddress);

		hook_Present->Enable();
		hook_ResizeTarget->Enable();

		RE::Main* main = RE::Main::GetSingleton();

		Cicero::GetSingleton()->SetupSinks();
		//ImmAssociateContextEx((HWND)main->wnd, NULL, NULL);
	}
}