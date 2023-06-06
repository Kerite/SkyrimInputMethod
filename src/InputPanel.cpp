#include "InputPanel.h"

#include "Cirero.h"
#include "Config.h"
#include "Helpers/DebugHelper.h"
#include "Utils.h"

IMEPanel::IMEPanel() :
	bEnabled(false),
	bDisableSpecialKey(false),
	m_bInitialized(false)
{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if (FAILED(hr)) {
		ERROR("Create D2D factory failed, HRESULT: {0:X}", hr);
	}
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWFactory));
	if (FAILED(hr)) {
		ERROR("Create DWrite factory failed, HRESULT: {0:X}", hr);
	}
	this->CalculatePosition();
}

bool IMEPanel::Initialize(IDXGISwapChain* a_pSwapChain)
{
	INFO("Initializing IMEPanel");
	HRESULT hr = S_OK;

	m_pSwapChain = a_pSwapChain;
	m_pSwapChain->AddRef();

	// Get Handle
	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc(&desc);
	hWindow = desc.OutputWindow;

	ImmAssociateContextEx(hWindow, NULL, 0);

	ReloadConfigs();
	hr = CreateD2DResources();

	if (FAILED(hr))
		ERROR("Initialize IMEPanel Failed {}", (uintptr_t)hWindow);

	m_bInitialized = SUCCEEDED(hr);
	return m_bInitialized;
}

void IMEPanel::ReloadConfigs()
{
	Configs* pConfigs = Configs::GetSingleton();
	m_position.fX = pConfigs->GetX();
	m_position.fY = pConfigs->GetY();
	m_wsFontName = pConfigs->GetFont();
	m_fontSizes.fCandidate = pConfigs->GetCandidateFontSize();
	m_fontSizes.fHeader = pConfigs->GetHeaderFontSize();
	m_fontSizes.fComposition = pConfigs->GetInputFontSize();
	DH_DEBUGW(L"IME Configs: fX: {}, fY: {}, font: {}", m_position.fX, m_position.fY, m_wsFontName);
	this->CalculatePosition();
}

void IMEPanel::NextGroup() {}

void IMEPanel::OnResetDevice() {}

void IMEPanel::OnLostDevice() {}

HRESULT IMEPanel::OnResizeTarget()
{
	return S_OK;
}

HRESULT IMEPanel::OnRender()
{
	HRESULT hr = S_OK;
	IDXGISurface* pBackBuffer;
	DXGI_SWAP_CHAIN_DESC swapDesc;

	if (!m_bInitialized) {
		return S_FALSE;
	}
	auto pControlMap = RE::ControlMap::GetSingleton();

	if (pControlMap->textEntryCount) {
		std::uint32_t iSelectedIndex = InterlockedCompareExchange(&ulSlectedIndex, ulSlectedIndex, -1);
		std::uint32_t iPageStartIndex = InterlockedCompareExchange(&ulPageStartIndex, ulPageStartIndex, -1);

		// Start actual Render
		hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

		if (SUCCEEDED(hr) && pBackBuffer) {
			D2D1_RENDER_TARGET_PROPERTIES props =
				D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_DEFAULT,
					D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

			hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer, &props, &m_pBackBufferRT);
		}
		if (SUCCEEDED(hr) && m_pBackBufferRT) {
			const WCHAR* text = wstrComposition.c_str();
			D2D1_SIZE_F targetSize = m_pBackBufferRT->GetSize();

			hr = this->CreateBrushes();

			if (SUCCEEDED(hr)) {
				csImeInformation.Enter();
				m_pBackBufferRT->BeginDraw();

				float item_height = m_fontSizes.fCandidate * 1.1 + 2 * m_position.fPadding;

				// 计算控件大小
				m_rectWidget = D2D1_ROUNDED_RECT(
					D2D1_RECT_F(
						m_position.fX,
						m_position.fY,
						m_position.fX + m_position.fWidth,
						m_rectComposition.bottom + item_height * vwsCandidateList.size() + m_position.fPadding),
					8.0f, 8.0f);

				// 填充背景
				m_pBackBufferRT->FillRoundedRectangle(m_rectWidget, m_pBackgroundBrush);
				// 绘制标题
				m_pBackBufferRT->DrawTextW(
					wstrInputMethodName.size() ? wstrInputMethodName.c_str() : L"Skyrim IME",
					wstrInputMethodName.size() ? wstrInputMethodName.size() : lstrlen(L"Skyrim IME"),
					m_pHeaderFormat,
					m_rectHeader,
					m_pHeaderColorBrush);
				// 绘制输入内容
				m_pBackBufferRT->DrawTextW(text, lstrlen(text), m_pInputContentFormat, m_rectComposition, m_pInputContentBrush);

				// 绘制候选字列表
				float fCurrentTopOffset = m_rectComposition.bottom;
				int i = 0;
				for (std::wstring wstrCurrentCandidate : vwsCandidateList) {
					// 当前选择
					auto rect = D2D1::RectF(
						m_position.fX + m_position.fPadding,
						fCurrentTopOffset,
						m_position.fX + m_position.fWidth - m_position.fPadding,
						fCurrentTopOffset + item_height);
					if (iSelectedIndex == i + iPageStartIndex) {
						// Draw selection border
						m_pBackBufferRT->DrawRoundedRectangle(D2D1_ROUNDED_RECT(rect, 4.0f), m_pSelectedColorBrush, 0.7f);
					}
					m_pBackBufferRT->DrawTextW(
						wstrCurrentCandidate.c_str(),
						wstrCurrentCandidate.size(),
						m_pCandicateItemFormat,
						D2D1::RectF(
							rect.left + m_position.fPadding,
							rect.top + m_position.fPadding,
							rect.right - m_position.fPadding,
							rect.bottom - m_position.fPadding),
						m_pCandidateColorBrush);
					fCurrentTopOffset += item_height;
					i++;
				}

				hr = m_pBackBufferRT->EndDraw();
				csImeInformation.Leave();
			}
		}
	}

	return hr;
}

HRESULT IMEPanel::CreateD2DResources()
{
	HRESULT hr = S_OK;
	DWRITE_TRIMMING trimmingOption{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
	if (SUCCEEDED(hr)) {
		// 标题字体
		hr = m_pDWFactory->CreateTextFormat(
			m_wsFontName.c_str(),
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.fHeader,
			L"zh-CN",
			&m_pHeaderFormat);
		m_pHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	}
	if (SUCCEEDED(hr)) {
		// 输入内容字体
		hr = m_pDWFactory->CreateTextFormat(
			m_wsFontName.c_str(),
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.fComposition,
			L"zh-CN",
			&m_pInputContentFormat);
	}
	if (SUCCEEDED(hr)) {
		// 候选字列表字体
		hr = m_pDWFactory->CreateTextFormat(
			m_wsFontName.c_str(),
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSizes.fCandidate,
			L"zh-CN",
			&m_pCandicateItemFormat);
		m_pCandicateItemFormat->SetTrimming(&trimmingOption, nullptr);
	}
	return hr;
}

HRESULT IMEPanel::CreateBrushes()
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

void IMEPanel::CalculatePosition()
{
	float fX = m_position.fX, fY = m_position.fY;
	float fWidth = m_position.fWidth;

	//m_rectWidget = D2D1::RoundedRect(D2D1::RectF(fX, fY, fX + fWidth, fY + height), 10.0f, 10.0f);
	m_rectHeader = D2D1::RectF(fX, fY, fX + fWidth, fY + m_fontSizes.fHeader + 2 * m_position.fPadding);
	m_rectComposition = D2D1::RectF(fX + m_position.fPadding, m_rectHeader.bottom, fX + fWidth, m_rectHeader.bottom + m_fontSizes.fComposition + 8.0f);
}

//bool IMEPanel::IsImeOpen()
//{
//	return ImmIsIME(GetKeyboardLayout(0));
//}