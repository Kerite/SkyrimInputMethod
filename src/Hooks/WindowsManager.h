#pragma once

#define WM_IME_SETSTATE 0x0655

// Used for hooking the window procedure
namespace Hooks
{
	class WindowsManager final : public Singleton<WindowsManager>
	{
	public:
		void Install(HWND);

	private:
		/*===========================
		==========  Hooks  ==========
		=============================*/

		struct WndProcHook
		{
			static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			static inline WNDPROC func;
		};
	};
}