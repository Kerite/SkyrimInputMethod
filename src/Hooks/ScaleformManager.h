#pragma once

namespace Hooks
{
	class ScaleformManager final : public Singleton<ScaleformManager>
	{
	public:
		void Install();

	private:
		class SKSEScaleform_AllowTextInput : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override;
		};

		CALL_DEF6(80302, 82325, 0x1D9, 0x1DD, Scaleform_SetScaleMode, void, RE::GFxMovieView*, RE::GFxMovieView::ScaleModeType);
	};
}