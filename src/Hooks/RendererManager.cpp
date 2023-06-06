#include "Hooks/RendererManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "Helpers/DebugHelper.h"
#include "InGameIme.h"
#include "Utils.h"

namespace Hooks
{
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags);
	REL::Relocation<decltype(IDXGISwapChain_Present)> old_IDXGISwapChain_Present;
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags)
	{
		InGameIME* pInGameIme = InGameIME::GetSingleton();

		pInGameIme->OnRender();

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

	HRESULT WINAPI RendererManager::DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked(
		IDXGIAdapter* a_pAdapter,
		D3D_DRIVER_TYPE a_driverType,
		HMODULE a_software,
		UINT a_flags,
		const D3D_FEATURE_LEVEL* a_pFeatureLevels,
		UINT a_featureLevels,
		UINT a_sdkVersion,
		const DXGI_SWAP_CHAIN_DESC* a_pSwapChainDesc,
		IDXGISwapChain** a_ppSwapChain,
		ID3D11Device** a_ppDevice,
		D3D_FEATURE_LEVEL* a_pFeatureLevel,
		ID3D11DeviceContext** a_ppImmediateContext)
	{
		DH_INFO("Creating D3D Devices and SwapChain using D3D11_CREATE_DEVICE_BGRA_SUPPORT flag");
		return oldFunc(
			a_pAdapter, a_driverType, a_software,
			a_flags | D3D11_CREATE_DEVICE_BGRA_SUPPORT, a_pFeatureLevels, a_featureLevels,
			a_sdkVersion, a_pSwapChainDesc, a_ppSwapChain,
			a_ppDevice, a_pFeatureLevel, a_ppImmediateContext);
	}

	void RendererManager::Renderer_Init_InitD3D_Hook::hooked()
	{
		DH_DEBUG("Calling origin Init3D");
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

		DH_INFO("Installing Present hook");
		hook_Present->Enable();
		DH_INFO("Installing ResizeTarget hook");
		hook_ResizeTarget->Enable();
	}

	void RendererManager::Install()
	{
		char name[MAX_PATH] = "\0";
		GetModuleFileNameA(GetModuleHandle(NULL), name, MAX_PATH);

		DH_INFO("Installing Renderer_Init_InitD3D hook");
		Utils::WriteCall<Renderer_Init_InitD3D_Hook>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));

		DH_INFO("Installing D3D11CreateDeviceAndSwapChain hook");
		{
			auto hook = dku::Hook::AddIATHook(
				name,
				"d3d11.dll",
				"D3D11CreateDeviceAndSwapChain",
				FUNC_INFO(DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked));
			DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::oldFunc =
				REL::Relocation<decltype(DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked)>(hook->OldAddress);
			hook->Enable();
		}

		INFO("Installed RendererManager");
	}
}