#pragma once

#include "Helpers/DebugHelper.h"

class ScaleformManager :
	public Singleton<ScaleformManager>,
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	void Install()
	{
		DH_INFO("Installing ScaleformManager");
		static bool bInstalled = false;
		if (bInstalled)
			return;
		RE::UI* pUI = RE::UI::GetSingleton();

		DH_INFO("Registering MenuOpenCloseEvent event");
		pUI->AddEventSink<RE::MenuOpenCloseEvent>(this);
		bInstalled = true;
		DH_INFO("Installed ScaleformManager");
	}

	void CopyText();

	volatile ULONG bConsoleOpenState;

private:
	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent*, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
};