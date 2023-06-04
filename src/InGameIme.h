#pragma once

#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string>
#include <vector>

#include "directxtk/CommonStates.h"
#include "directxtk/SpriteFont.h"

#include "ICriticalSection.h"

typedef std::vector<std::wstring> CandidateList;
#define MAX_CANDLIST 8

#define SCI_ES_DISABLE 0

struct IMEPosition
{
	float x;
	float y;
	float width;
	float height;

	IMEPosition() :
		x(15.0f), y(15.0f),
		width(200.0f), height(300.0f)
	{}
};

struct IMEFontSizes
{
	float header;
	float inputContent;
	float candidateItem;

	IMEFontSizes() :
		header(15.0f),
		inputContent(12.0f),
		candidateItem(10.0f) {}
};

struct RendererData
{
	ID3D11Device* pD3DDevice;
	ID3D11DeviceContext* pD3DDeviceContext;
	IDXGIFactory* pFactory;
	IDXGISwapChain* pSwapChain;
};

class InGameIME final : public Singleton<InGameIME>
{
public:
	InGameIME();

	bool Initialize(IDXGISwapChain* swapchain, ID3D11Device* a_device, ID3D11DeviceContext* a_device_context);

	void OnLoadConfig();

	// D3D Events
	void NextGroup();
	void OnResetDevice();
	void OnLostDevice();
	HRESULT OnRender();
	HRESULT OnResizeTarget();

	volatile std::uintptr_t enableState = SCI_ES_DISABLE;
	volatile std::uintptr_t disableKeyState = false;
	volatile std::uintptr_t pageStartIndex = 0;
	volatile std::uintptr_t selectedIndex = -1;

	std::string stateInfo;
	/// <summary>
	/// 输入的内容, 需要调用DrawTextW()来绘制，所以是宽字符
	/// </summary>
	std::wstring inputContent;
	/// <summary>
	/// 候选字列表
	/// </summary>
	CandidateList candidateList;

	bool IsImeOpen();
	[[nodiscard]] inline bool Initialized() { return mInitialized; }

	void ProcessImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void ProcessImeNotify(HWND hWnd, WPARAM wParam, LPARAM lParam);

	HWND hwnd;
	ICriticalSection ime_critical_section;

private:
	bool mInitialized;

	IMEPosition m_position;
	IMEFontSizes m_fontSizes;

	IDWriteTextFormat* pHeaderFormat;
	IDWriteTextFormat* pInputContentFormat;
	IDWriteTextFormat* pCandicateItemFormat;

	D2D1_ROUNDED_RECT m_widgetRect;
	D2D_RECT_F m_headerRect;
	D2D_RECT_F m_inputContentRect;
	D2D_RECT_F m_basecandidateItemRect;

	ID2D1SolidColorBrush* m_pBackgroundBrush;
	ID2D1SolidColorBrush* m_pHeaderColorBrush;
	ID2D1SolidColorBrush* m_pInputContentBrush;
	ID2D1SolidColorBrush* m_pCandidateColorBrush;
	ID2D1SolidColorBrush* m_pSelectedColorBrush;

	ID2D1Factory* m_pD2DFactory;
	IDWriteFactory* m_pDWFactory;
	ID2D1RenderTarget* m_pBackBufferRT = nullptr;
	D2D1_POINT_2F origin;

	RendererData* rd;

	/// <summary>
	///  创建D2D相关资源，包括字体
	/// </summary>
	HRESULT CreateD2DResources();

	/// 创建笔刷
	HRESULT CreateBrushes();

	/// 计算位置
	void CalculatePosition();
};