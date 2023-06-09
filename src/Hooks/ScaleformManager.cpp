#include "Hooks/ScaleformManager.h"

#include "Hooks/InputManager.h"
#include "Utils.h"

void Hooks::ScaleformManager::SKSEScaleform_AllowTextInput::Call(Params& a_params)
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

void _fastcall Hooks::ScaleformManager::Hook_Scaleform_SetScaleMode::hooked(RE::GFxMovieView* a_pView, RE::GFxMovieView::ScaleModeType a_eScaleMode)
{
	DEBUG("Calling Original RegisterScaleformFunctions");
	oldFunc(a_pView, a_eScaleMode);

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

void Hooks::ScaleformManager::Install()
{
	INFO("Installing ScaleformManager");

	INFO("Hooking BSScaleformManager::LoadMovie")
	Hook_Scaleform_SetScaleMode::Install();

	INFO("Installed ScaleformManager");
}