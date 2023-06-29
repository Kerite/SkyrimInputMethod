#pragma once

#include <d3d11.h>

namespace Hooks
{
	// Used for hooking renderer
	class RendererManager final : public Singleton<RendererManager>
	{
	public:
		void Install();

	private:
		std::atomic_bool m_bInitialized{ false };

		// From https://github.com/max-su-2019/MaxsuDetectionMeter/blob/main/src/Renderer.h
		HOOK_DEF(75595, 77226, 0x9, 0x275, InitD3D, void);
		HOOK_DEF(75461, 77246, 0x9, 0x9, Present, void, std::uint32_t);

		struct Renderer_Init_InitD3D_Hook
		{
			static void hooked();

			static inline REL::Relocation<decltype(&hooked)> oldFunc;
		};

		struct DLL_D3D11_D3D11CreateDeviceAndSwapChain_Hook
		{
			static HRESULT WINAPI hooked(
				IDXGIAdapter* a_pAdapter,
				D3D_DRIVER_TYPE a_driverType,
				HMODULE a_software,
				UINT a_flags,
				const D3D_FEATURE_LEVEL* a_pFeatureLevels,
				UINT a_featureLevels,
				UINT a_sdkVersion,
				const DXGI_SWAP_CHAIN_DESC* a_pSwapChainDesc,
				IDXGISwapChain** ppSwapChain,
				ID3D11Device** ppDevice,
				D3D_FEATURE_LEVEL* pFeatureLevel,
				ID3D11DeviceContext** ppImmediateContext);
			static inline REL::Relocation<decltype(hooked)> oldFunc;
		};
	};
}