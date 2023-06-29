#include "Hooks/RendererManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "Helpers/DebugHelper.h"
#include "InputPanel.h"
#include "Utils.h"

#pragma region Hooks_Impl
void Hooks::RendererManager::Hook_InitD3D::hooked()
{
	INFO("Calling origin InitD3D");
	oldFunc();
	auto pRendererManager = RE::BSGraphics::Renderer::GetSingleton();
	if (!pRendererManager) {
		ERROR("[Renderer] Failed get rendererManager");
	}

	auto renderData = pRendererManager->data;
	auto pSwapChain = renderData.renderWindows->swapChain;
	auto pDevice = renderData.forwarder;
	auto pDeviceContext = renderData.context;
	if (!pSwapChain) {
		ERROR("[Renderer] Failed get SwapChain");
	}
	IMEPanel::GetSingleton()->Initialize(pSwapChain);

	DXGI_SWAP_CHAIN_DESC desc{};
	if (FAILED(pSwapChain->GetDesc(&desc))) {
		ERROR("[Renderer] Failed get DXGI_SWAP_CHAIN_DESC");
	}

	ImGui::CreateContext();

	std::ifstream fontconfig("Interface/fontconfig.txt");
	std::string fontconfigStr;
	if (fontconfig) {
		std::ostringstream ss;
		ss << fontconfig.rdbuf();
		fontconfigStr = ss.str();
	}

	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder glyphBuilder;
	glyphBuilder.AddText(fontconfigStr.c_str());
	glyphBuilder.BuildRanges(&ranges);

	if (!ImGui_ImplDX11_Init(pDevice, pDeviceContext)) {
		ERROR("[Renderer] Failed init imgui (DX11)")
	}
	if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
		ERROR("[Renderer] Failed init imgui (Win32)")
	}
	RendererManager::GetSingleton()->m_bInitialized.store(true);
}

void Hooks::RendererManager::Hook_Present::hooked(std::uint32_t a_p1)
{
	oldFunc(a_p1);
	INFO("Present()");
	if (!RendererManager::GetSingleton()->m_bInitialized.load()) {
		return;
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	IMEPanel::GetSingleton()->OnRender();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
#pragma endregion

namespace Hooks
{
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags);
	REL::Relocation<decltype(IDXGISwapChain_Present)> old_IDXGISwapChain_Present;
	HRESULT WINAPI IDXGISwapChain_Present(IDXGISwapChain* a_pThis, UINT a_syncInterval, UINT a_flags)
	{
		IMEPanel* pIMEPanel = IMEPanel::GetSingleton();

		pIMEPanel->OnRender();

		return old_IDXGISwapChain_Present(a_pThis, a_syncInterval, a_flags);
	}

	HRESULT WINAPI IDXGISwapChain_ResizeTarget(IDXGISwapChain* a_pThis, const DXGI_MODE_DESC* a_pNewTargetParameters);
	REL::Relocation<decltype(IDXGISwapChain_ResizeTarget)> old_IDXGISwapChain_ResizeTarget;
	HRESULT WINAPI IDXGISwapChain_ResizeTarget(IDXGISwapChain* a_pThis, const DXGI_MODE_DESC* a_pNewTargetParameters)
	{
		HRESULT hr = old_IDXGISwapChain_ResizeTarget(a_pThis, a_pNewTargetParameters);
		if (SUCCEEDED(hr)) {
			IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
			hr = pIMEPanel->OnResizeTarget();
		}
		return hr;
	}

	HRESULT WINAPI RendererManager::DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked(
		IDXGIAdapter* a_pAdapter,
		D3D_DRIVER_TYPE a_eDriverType,
		HMODULE a_hSoftware,
		UINT a_uFlags,
		const D3D_FEATURE_LEVEL* a_pFeatureLevels,
		UINT a_uiFeatureLevels,
		UINT a_uiSdkVersion,
		const DXGI_SWAP_CHAIN_DESC* a_pSwapChainDesc,
		IDXGISwapChain** a_ppSwapChain,
		ID3D11Device** a_ppDevice,
		D3D_FEATURE_LEVEL* a_pFeatureLevel,
		ID3D11DeviceContext** a_ppImmediateContext)
	{
		DH_INFO("Creating D3D Devices and SwapChain using D3D11_CREATE_DEVICE_BGRA_SUPPORT flag");
		return oldFunc(
			a_pAdapter, a_eDriverType, a_hSoftware,
			a_uFlags | D3D11_CREATE_DEVICE_BGRA_SUPPORT, a_pFeatureLevels, a_uiFeatureLevels,
			a_uiSdkVersion, a_pSwapChainDesc, a_ppSwapChain,
			a_ppDevice, a_pFeatureLevel, a_ppImmediateContext);
	}

	void RendererManager::Renderer_Init_InitD3D_Hook::hooked()
	{
		DH_DEBUG("Calling origin Init3D");
		oldFunc();

		auto pRenderer = RE::BSGraphics::Renderer::GetSingleton();
		IMEPanel* pIMEPanel = IMEPanel::GetSingleton();

		IDXGISwapChain* pDXGISwapChain = pRenderer->data.renderWindows->swapChain;
		//ID3D11Device* pD3DDevice = pRenderer->data.forwarder;
		//ID3D11DeviceContext* pD3DDeviceContext = pRenderer->data.context;

		pIMEPanel->Initialize(pDXGISwapChain);

		DH_INFO("Installing Present hook");
		auto hook_Present = dku::Hook::AddVMTHook(pDXGISwapChain, 8, FUNC_INFO(IDXGISwapChain_Present));
		old_IDXGISwapChain_Present = REL::Relocation<decltype(IDXGISwapChain_Present)>(hook_Present->OldAddress);
		hook_Present->Enable();

		DH_INFO("Installing ResizeTarget hook");
		auto hook_ResizeTarget = dku::Hook::AddVMTHook(pDXGISwapChain, 14, FUNC_INFO(IDXGISwapChain_ResizeTarget));
		old_IDXGISwapChain_ResizeTarget = REL::Relocation<decltype(IDXGISwapChain_ResizeTarget)>(hook_ResizeTarget->OldAddress);
		hook_ResizeTarget->Enable();
	}

	void RendererManager::Install()
	{
		char name[MAX_PATH] = "\0";
		GetModuleFileNameA(GetModuleHandle(NULL), name, MAX_PATH);

		DH_INFO("Installing Renderer_Init_InitD3D hook");
		//Utils::WriteCall<Renderer_Init_InitD3D_Hook>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		Utils::Hook::WriteCall<Hook_InitD3D>();

		Utils::Hook::WriteCall<Hook_Present>();

		DH_INFO("Installing D3D11CreateDeviceAndSwapChain@d3d11.dll hook");
		auto hook = dku::Hook::AddIATHook(
			name,
			"d3d11.dll",
			"D3D11CreateDeviceAndSwapChain",
			FUNC_INFO(DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked));
		DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::oldFunc =
			REL::Relocation<decltype(DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook::hooked)>(hook->OldAddress);
		hook->Enable();

		INFO("Installed RendererManager");
	}
}