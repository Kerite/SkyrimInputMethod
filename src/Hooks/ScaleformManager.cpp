#include "Hooks/ScaleformManager.h"

#include "Hooks/InputManager.h"
#include "RE/Offset.h"
#include "Utils.h"

namespace Hooks
{
	void ScaleformManager::Install()
	{
		INFO("Installing ScaleformManager");
		auto hook = REL::Relocation<std::uintptr_t>(RE::Offset::BSScaleformManager::LoadMovie, REL::Relocate(0x1D9, 0x1DD));
		INFO("Hooking BSScaleformManager::LoadMovie")
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);
		auto ptr = trampoline.write_call<6>(hook.address(), &RegisterScaleformFunctions);
		_SetViewScaleMode = *reinterpret_cast<std::uintptr_t*>(ptr);
		INFO("Installed ScaleformManager");
	}

	void ScaleformManager::RegisterScaleformFunctions(RE::GFxMovieView* a_pView, RE::GFxMovieView::ScaleModeType a_eScaleMode)
	{
		DEBUG("Calling Original RegisterScaleformFunctions");
		_SetViewScaleMode(a_pView, a_eScaleMode);

		if (!a_pView) {
			return;
		}

		if (a_pView->GetMovieDef()) {
			DEBUG("Replacing AllowTextInput of {}", a_pView->GetMovieDef()->GetFileURL());
		}

		RE::GFxValue skse;
		a_pView->GetVariable(&skse, "_global.skse");

		if (!skse.IsObject()) {
			ERROR("Failed to get _global.skse");
		}
		RE::GFxValue fn_AllowTextInput;
		static auto AllowTextInput = new SKSEScaleform_AllowTextInput;
		a_pView->CreateFunction(&fn_AllowTextInput, AllowTextInput);
		skse.SetMember("AllowTextInput", fn_AllowTextInput);
	}

	void ScaleformManager::SKSEScaleform_AllowTextInput::Call(Params& a_params)
	{
		using namespace RE;

		ControlMap* pControlMap = ControlMap::GetSingleton();
		InputManager* pInputManager = InputManager::GetSingleton();

		if (a_params.argCount == 0) {
			spdlog::error("ArgCount of AllowTextInput is zero");
			return;
		}
		bool argEnable = a_params.args[0].GetBool();

		if (!pControlMap || !pInputManager) {
			spdlog::error("Get ControlMap or InputManager failed");
		}
		pInputManager->ProcessAllowTextInput(argEnable);
		pControlMap->AllowTextInput(argEnable);
	}
}