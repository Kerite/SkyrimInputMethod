#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include "directxtk/DirectXHelpers.h"
#include "directxtk/SimpleMath.h"
#include "directxtk/SpriteBatch.h"

#include "Cirero.h"
#include "Config.h"
#include "Helpers/DebugHelper.h"
#include "ICriticalSection.h"
#include "InGameIme.h"
#include "Offsets.h"
#include "Utils.h"

#define TextFont L"微软雅黑"
#define TextSize 20
#define SPACE_BETWEEN_CANDIDATE 10

using namespace DirectX;

InGameIME::InGameIME() :
	enableState(false),
	disableKeyState(false),
	mInitialized(false)
{
	HRESULT hr = S_OK;

	rd = new RendererData();
	memset(rd, 0, sizeof(RendererData));

	if (FAILED(hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory))) {
		ERROR("Create d2d factory failed, HRESULT: {0:X}", hr);
	}
	if (FAILED(hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWFactory)))) {
		ERROR("Create dw factory failed, HRESULT: {0:X}", hr);
	}
	this->CalculatePosition();
}

bool InGameIME::Initialize(IDXGISwapChain* a_pSwapChain, ID3D11Device* a_pDevice, ID3D11DeviceContext* a_pDeviceContext)
{
	INFO("Initializing InGameIME");
	HRESULT hr = S_OK;

	rd->pSwapChain = a_pSwapChain;
	rd->pSwapChain->AddRef();

	{
		// Get Handle
		DXGI_SWAP_CHAIN_DESC desc;
		rd->pSwapChain->GetDesc(&desc);
		hwnd = desc.OutputWindow;

		ImmAssociateContextEx(hwnd, 0, 0);
	}

	CreateD2DResources();

	if (FAILED(hr))
		ERROR("Initialize InGameIME Failed {}", (uintptr_t)hwnd);

	mInitialized = SUCCEEDED(hr);
	return mInitialized;
}

void InGameIME::OnLoadConfig()
{
	this->CalculatePosition();
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

		// Start actual Render
		hr = rd->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

		if (SUCCEEDED(hr) && pBackBuffer) {
			D2D1_RENDER_TARGET_PROPERTIES props =
				D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_DEFAULT,
					D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

			hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer, &props, &m_pBackBufferRT);
		} else {
			ERROR("Get Buffer Failed, HRESULT: {:#X}", (ULONG)hr);
		}
		if (SUCCEEDED(hr) && m_pBackBufferRT) {
			const WCHAR* text = inputContent.c_str();
			D2D1_SIZE_F targetSize = m_pBackBufferRT->GetSize();

			hr = this->CreateBrushes();

			if (SUCCEEDED(hr)) {
				ime_critical_section.Enter();
				m_pBackBufferRT->BeginDraw();

				// 填充背景
				m_pBackBufferRT->FillRoundedRectangle(m_widgetRect, m_pBackgroundBrush);

				// 绘制标题
				m_pBackBufferRT->DrawTextW(L"中文输入", lstrlen(L"中文输入"), pHeaderFormat, m_headerRect, m_pHeaderColorBrush);
				// 绘制输入内容
				m_pBackBufferRT->DrawTextW(text, lstrlen(text), pInputContentFormat, m_inputContentRect, m_pInputContentBrush);

				// 绘制候选字列表
				float y_top_offset = m_inputContentRect.bottom;
				float item_height = m_fontSizes.candidateItem + 8.0f;
				int i = 0;
				for (auto candidate : candidateList) {
					if (iSelectedIndex == i + iPageStartIndex) {
						m_pBackBufferRT->DrawTextW(
							candidate.c_str(),
							candidate.size(),
							pCandicateItemFormat,
							D2D1::RectF(m_position.x, y_top_offset, m_position.x + m_position.width, y_top_offset + item_height),
							m_pSelectedColorBrush);
					} else {
						m_pBackBufferRT->DrawTextW(
							candidate.c_str(),
							candidate.size(),
							pCandicateItemFormat,
							D2D1::RectF(m_position.x, y_top_offset, m_position.x + m_position.width, y_top_offset + item_height),
							m_pCandidateColorBrush);
					}
					y_top_offset += item_height;
					i++;
				}

				hr = m_pBackBufferRT->EndDraw();
				ime_critical_section.Leave();
			}
		} else {
			ERROR("Get BackBufferRT Failed, HRESULT: {:#X}", (ULONG)hr);
		}
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
		if (pControlMap->textEntryCount && !pCicero->ciceroState) {
			Utils::UpdateCandidateList(hWnd);
		}
	}
}

HRESULT InGameIME::CreateD2DResources()
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr)) {
		// 标题字体
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.header,
			L"zh-CN",
			&pHeaderFormat);
		pHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	}
	if (SUCCEEDED(hr)) {
		// 输入内容字体
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.inputContent,
			L"zh-CN",
			&pInputContentFormat);
	}
	if (SUCCEEDED(hr)) {
		// 候选字列表字体
		hr = m_pDWFactory->CreateTextFormat(
			TextFont,
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.candidateItem,
			L"en-US",
			&pCandicateItemFormat);
	}
	return hr;
}

HRESULT InGameIME::CreateBrushes()
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 0.75f), &m_pBackgroundBrush);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pHeaderColorBrush);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pInputContentBrush);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pCandidateColorBrush);
	}
	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkBlue), &m_pSelectedColorBrush);
	}
	if (FAILED(hr)) {
		ERROR("Create Brushes Failed");
	}
	return hr;
}

void InGameIME::CalculatePosition()
{
	float x = m_position.x, y = m_position.y;
	float width = m_position.width, height = m_position.height;

	m_widgetRect = D2D1::RoundedRect(D2D1::RectF(x, y, x + width, y + height), 10.0f, 10.0f);
	m_headerRect = D2D1::RectF(x, y, x + width, y + m_fontSizes.header + 8.0f);
	m_inputContentRect = D2D1::RectF(x, m_headerRect.bottom, x + width, m_headerRect.bottom + m_fontSizes.inputContent + 8.0f);
}