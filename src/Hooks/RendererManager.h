#pragma once

namespace Hooks
{
	class RendererManager final : public Singleton<RendererManager>
	{
	public:
		void Install();

	private:
		struct Renderer_Init_InitD3D_Hook
		{
			static void hooked();

			static inline REL::Relocation<decltype(&hooked)> oldFunc;
		};
	};
}