#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include <d2d1.h>
#include <d3d11.h>

#include "directxtk/DirectXHelpers.h"
#include "directxtk/SimpleMath.h"
#include "directxtk/SpriteBatch.h"

#include "Cirero.h"
#include "Config.h"
#include "ICriticalSection.h"
#include "InGameIme.h"
#include "Offsets.h"
#include "Utils.h"

#define TextFont L"微软雅黑"
#define TextSize 20
#define SPACE_BETWEEN_CANDIDATE 10

using namespace DirectX;

ICriticalSection ime_critical_section;

InGameIME::InGameIME()
{
	XMFLOAT2A tmp(10, 10);

	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if (SUCCEEDED(hr)) {
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWFactory));
	}
}

bool InGameIME::Initialize(IDXGISwapChain* a_pSwapChain, ID3D11Device* a_pDevice, ID3D11DeviceContext* a_pDeviceContext)
{
	INFO("Initializing InGameIME");
	HRESULT hr = S_OK;

	enableState = false;
	disableKeyState = false;

	pDevice = a_pDevice;
	pDeviceContext = a_pDeviceContext;

	// From imgui, Get DXGI Factory
	IDXGIDevice* pDXGIDevice = nullptr;
	IDXGIAdapter* pDXGIAdapter = nullptr;
	IDXGIFactory* pDXGIFactory = nullptr;
	IDXGISurface* pDXGISurface = nullptr;

	hr = a_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));
	if (SUCCEEDED(hr)) {
		hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);
	}
	if (SUCCEEDED(hr)) {
		hr = pDXGIAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
	}
	if (SUCCEEDED(hr)) {
		pFactory = pDXGIFactory;
	}

	pSwapChain = a_pSwapChain;

	// Get Handle
	DXGI_SWAP_CHAIN_DESC desc;
	pSwapChain->GetDesc(&desc);
	hwnd = desc.OutputWindow;
	DEBUG("Handle of OutputWindow: {}", (uintptr_t)hwnd);

	// Create TextFormats
	if (SUCCEEDED(hr)) {
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			22.0F,
			L"en-US",
			&pCandicateItemFormat);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			22.0f,
			L"zh-CN",
			&pInputContentFormat);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			22.0f,
			L"zh-CN",
			&pHeaderFormat);
	}
	if (FAILED(hr))
		ERROR("Initialize InGameIme Failed {}", (uintptr_t)hwnd);

	mInitialized = SUCCEEDED(hr);
	return mInitialized;
}

void InGameIME::NextGroup() {}

void InGameIME::OnResetDevice() {}

void InGameIME::OnLostDevice()
{
}

HRESULT InGameIME::OnResizeTarget()
{
	return S_OK;
}

/// <summary>
/// Call on every frame, Draw candidate box.
/// Called by RendererManager
/// </summary>
HRESULT InGameIME::OnRender()
{
	HRESULT hr = S_OK;
	IDXGISurface* pBackBuffer;
	DXGI_SWAP_CHAIN_DESC swapDesc;

	if (!mInitialized) {
		return S_FALSE;
	}
	auto control_map = RE::ControlMap::GetSingleton();

	if (control_map->textEntryCount && InterlockedCompareExchange(&enableState, enableState, 2)) {
		std::uint32_t iSelectedIndex = InterlockedCompareExchange(&selectedIndex, selectedIndex, -1);
		std::uint32_t iPageStartIndex = InterlockedCompareExchange(&pageStartIndex, pageStartIndex, -1);

		ime_critical_section.Enter();

		const WCHAR* text = inputContent.c_str();

		// Start actual Render
		hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

		if (SUCCEEDED(hr)) {
			D2D1_RENDER_TARGET_PROPERTIES props =
				D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_DEFAULT,
					D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

			hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer, &props, &m_pBackBufferRT);
		}
		if (SUCCEEDED(hr) && m_pBackBufferRT) {
			D2D1_SIZE_F targetSize = m_pBackBufferRT->GetSize();
			hr = m_pBackBufferRT->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF(0xFF6432, 1.0f)),
				&m_pWhiteColorBrush);
			if (SUCCEEDED(hr)) {
				hr = m_pBackBufferRT->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::Black),
					&m_pHeaderColorBrush);
			}

			if (SUCCEEDED(hr)) {
				m_pBackBufferRT->BeginDraw();

				// 填充背景
				m_pBackBufferRT->FillRoundedRectangle(widgetRect, m_pBackgroundBrush);

				m_pBackBufferRT->DrawTextW(
					L"中文输入",
					lstrlen(L"中文输入"),
					pHeaderFormat,
					headerRect,
					m_pWhiteColorBrush);
				m_pBackBufferRT->DrawTextW(
					text,
					lstrlen(text),
					pInputContentFormat,
					inputContentRect,
					m_pWhiteColorBrush);

				hr = m_pBackBufferRT->EndDraw();
			}
		}

		ime_critical_section.Leave();
	}

	return hr;
}

bool InGameIME::IsImeOpen()
{
	return ImmIsIME(GetKeyboardLayout(0));
}

/// <summary>
/// 处理 WM_IME_COMPOSITION
/// </summary>
/// <param name="hWnd"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
void InGameIME::ProcessImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	RE::ControlMap* control_map = RE::ControlMap::GetSingleton();
	// 当前正在输入的输入法句柄
	HIMC hImc = ImmGetContext(hWnd);

	if (!hImc) {
		return;
	}

	// 正在输入字符
	if (control_map->textEntryCount) {
		if (lParam & GCS_RESULTSTR) {
			// 获取结果字符串长度。单位是字节
			DWORD bufferSize = ImmGetCompositionString(hImc, GCS_RESULTSTR, NULL, 0);
			// 加上字符串结尾的\0
			bufferSize += sizeof(WCHAR);
			// 给结果字符串分配内存
			std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> resultBuffer(reinterpret_cast<WCHAR*>(Utils::HeapAlloc(bufferSize)), Utils::HeapFree);
			ZeroMemory(resultBuffer.get(), bufferSize);
			// 重新获取一次结果字符串
			ImmGetCompositionString(hImc, GCS_RESULTSTR, resultBuffer.get(), bufferSize);
			// 将结果字符串逐字发送到Scaleform消息队列
			for (size_t i = 0; i < bufferSize; i++) {
				WCHAR unicode = resultBuffer[i];
				Utils::SendUnicodeMessage(unicode);
			}

			ImmReleaseContext(hWnd, hImc);
		}
		if (lParam & GCS_COMPSTR) {
			Utils::GetInputString(hWnd);
		}
		if (lParam & CS_INSERTCHAR) {
		}
		if (lParam & GCS_CURSORPOS) {
		}
	}
	return;
}

void InGameIME::ProcessImeNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	DEBUG("[WinProc] 处理 IME_NOTIFY 消息 WPARAM: {:X}", wParam);
	RE::ControlMap* pControlMap = RE::ControlMap::GetSingleton();
	Cicero* pCicero = Cicero::GetSingleton();

	switch (wParam) {
	case IMN_OPENCANDIDATE:
	case IMN_SETCANDIDATEPOS:
	case IMN_CHANGECANDIDATE:
		InterlockedExchange(&enableState, 1);
		DEBUG("TextEntryCount: {}", pControlMap->textEntryCount)
		if (pControlMap->textEntryCount && pCicero->ciceroState == CICERO_DISABLED) {
			Utils::GetCandidateList(hWnd);
		}
	}
}

HRESULT InGameIME::CreateD2DResources()
{
	return E_NOTIMPL;
}
