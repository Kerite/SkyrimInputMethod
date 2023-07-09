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
		CALL_DEF(75595, 77226, 0x9, 0x275, InitD3D, void);
		CALL_DEF(75461, 77246, 0x9, 0x9, D3D_Present, void, std::uint32_t);
	};
}