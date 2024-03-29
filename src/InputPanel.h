#pragma once

#include <wrl/client.h>

#include "CCriticalSection.h"

typedef std::vector<std::wstring> CandidateList;

#define SCI_ES_DISABLE 0

// IMEPanel->bEnabled values

// Don't draw UI
#define IME_UI_DISABLED 0L
// Draw UI in game
#define IME_UI_ENABLED 1L

extern RTL_CRITICAL_SECTION g_rcsPanelCriticalSection;

// Used for cache IME informations
class IMEPanel final : public Singleton<IMEPanel>
{
public:
	IMEPanel() :
		hWindow(nullptr){};

	/// Render IME panel
	void OnRender();
	void Initialize(HWND hWnd);

	volatile BOOL uiState = IME_UI_DISABLED;

	// Whether allow special key been processed by Scaleform MessageQueue
	std::atomic_bool m_bDisableSpecialKey{ false };
	std::atomic_bool m_bEnabled{ false };
	std::atomic_uint32_t m_ulPageStartIndex{ 0 };
	std::atomic_uint32_t m_ulSlectedIndex{ 0 };
	std::atomic_uint32_t m_ulCursorPos{ 0 };

	std::wstring wstrInputMethodName;
	std::wstring wstrComposition;

	// Candidate list
	CandidateList vwsCandidateList;

	CCriticalSection csImeInformation;

	HWND hWindow;
};