#include "InGameIme.h"
#include "Cirero.h"
#include "Config.h"
#include "Helpers/DebugHelper.h"
#include "ICriticalSection.h"
#include "Offsets.h"
#include "Utils.h"

using namespace DirectX;

InGameIME::InGameIME() :
	bEnabled(false),
	bDisableSpecialKey(false),
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

		ImmAssociateContextEx(hwnd, NULL, 0);
	}
	OnLoadConfig();
	CreateD2DResources();

	if (FAILED(hr))
		ERROR("Initialize InGameIME Failed {}", (uintptr_t)hwnd);

	mInitialized = SUCCEEDED(hr);
	return mInitialized;
}

void InGameIME::OnLoadConfig()
{
	Configs* pConfigs = Configs::GetSingleton();
	m_position.x = pConfigs->GetX();
	m_position.y = pConfigs->GetY();
	fontName = pConfigs->GetFont();
	m_fontSizes.candidateItem = pConfigs->GetCandidateFontSize();
	m_fontSizes.header = pConfigs->GetHeaderFontSize();
	m_fontSizes.inputContent = pConfigs->GetInputFontSize();
	DH_DEBUGW(L"IME Configs: x: {}, y: {}, font: {}", m_position.x, m_position.y, fontName);
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

	if (control_map->textEntryCount) {
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
		}
		if (SUCCEEDED(hr) && m_pBackBufferRT) {
			const WCHAR* text = inputContent.c_str();
			D2D1_SIZE_F targetSize = m_pBackBufferRT->GetSize();

			hr = this->CreateBrushes();

			if (SUCCEEDED(hr)) {
				imeCriticalSection.Enter();
				m_pBackBufferRT->BeginDraw();

				float item_height = m_fontSizes.candidateItem * 1.1 + 2 * m_position.padding;

				// 计算控件大小
				m_widgetRect = D2D1_ROUNDED_RECT(
					D2D1_RECT_F(
						m_position.x,
						m_position.y,
						m_position.x + m_position.width,
						m_inputContentRect.bottom + item_height * candidateList.size() + m_position.padding),
					8.0f, 8.0f);

				// 填充背景
				m_pBackBufferRT->FillRoundedRectangle(m_widgetRect, m_pBackgroundBrush);
				// 绘制标题
				m_pBackBufferRT->DrawTextW(currentMethodName.c_str(), currentMethodName.size(), pHeaderFormat, m_headerRect, m_pHeaderColorBrush);
				// 绘制输入内容
				m_pBackBufferRT->DrawTextW(text, lstrlen(text), pInputContentFormat, m_inputContentRect, m_pInputContentBrush);

				// 绘制候选字列表
				float y_top_offset = m_inputContentRect.bottom;
				int i = 0;
				for (auto candidate : candidateList) {
					// 当前选择
					auto rect = D2D1::RectF(
						m_position.x + m_position.padding,
						y_top_offset,
						m_position.x + m_position.width - m_position.padding,
						y_top_offset + item_height);
					if (iSelectedIndex == i + iPageStartIndex) {
						// Draw selection border
						m_pBackBufferRT->DrawRoundedRectangle(D2D1_ROUNDED_RECT(rect, 4.0f), m_pSelectedColorBrush, 0.7f);
					}
					m_pBackBufferRT->DrawTextW(
						candidate.c_str(),
						candidate.size(),
						pCandicateItemFormat,
						D2D1::RectF(
							rect.left + m_position.padding,
							rect.top + m_position.padding,
							rect.right - m_position.padding,
							rect.bottom - m_position.padding),
						m_pCandidateColorBrush);
					y_top_offset += item_height;
					i++;
				}

				hr = m_pBackBufferRT->EndDraw();
				imeCriticalSection.Leave();
			}
		}
	}

	return hr;
}

bool InGameIME::IsImeOpen()
{
	return ImmIsIME(GetKeyboardLayout(0));
}

HRESULT InGameIME::CreateD2DResources()
{
	HRESULT hr = S_OK;
	DWRITE_TRIMMING trimmingOption{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
	if (SUCCEEDED(hr)) {
		// 标题字体
		hr = m_pDWFactory->CreateTextFormat(
			fontName.c_str(),
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
			fontName.c_str(),
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
			fontName.c_str(),
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.candidateItem,
			L"zh-CN",
			&pCandicateItemFormat);
		pCandicateItemFormat->SetTrimming(&trimmingOption, nullptr);
	}
	return hr;
}

HRESULT InGameIME::CreateBrushes()
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr)) {
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 0.80f), &m_pBackgroundBrush);
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
		hr = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &m_pSelectedColorBrush);
	}
	if (FAILED(hr)) {
		ERROR("Create Brushes Failed");
	}
	return hr;
}

void InGameIME::CalculatePosition()
{
	float x = m_position.x, y = m_position.y;
	float width = m_position.width;

	//m_widgetRect = D2D1::RoundedRect(D2D1::RectF(x, y, x + width, y + height), 10.0f, 10.0f);
	m_headerRect = D2D1::RectF(x, y, x + width, y + m_fontSizes.header + 2 * m_position.padding);
	m_inputContentRect = D2D1::RectF(x + m_position.padding, m_headerRect.bottom, x + width, m_headerRect.bottom + m_fontSizes.inputContent + 8.0f);
}