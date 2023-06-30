#pragma once

namespace Hooks
{
	class ScaleformManager final : public Singleton<ScaleformManager>
	{
	public:
		void Install();

	private:
		static void RegisterScaleformFunctions(RE::GFxMovieView* a_view, RE::GFxMovieView::ScaleModeType a_scaleMode);

		inline static REL::Relocation<decltype(&RegisterScaleformFunctions)> _SetViewScaleMode;
		RE::GFxMovie* _movie = nullptr;

		class SKSEScaleform_AllowTextInput : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override;
		};
	};
}