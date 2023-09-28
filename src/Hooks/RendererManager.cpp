#include "Hooks/RendererManager.h"

#include "detours/detours.h"

#include "Cirero.h"
#include "Config.h"
#include "InputPanel.h"
#include "Utils.h"
#include "WindowsManager.h"

std::string SafeGetFont(std::string_view fontPathOrName)
{
	if (std::filesystem::exists(fontPathOrName)) {
		return std::string(fontPathOrName);
	}
	std::string systemFont = std::string("C:\\Windows\\Fonts\\").append(fontPathOrName.data());
	if (std::filesystem::exists(systemFont)) {
		return systemFont;
	}
	std::string skseFont = std::string("Data\\SKSE\\Plugins\\").append(fontPathOrName.data());
	if (std::filesystem::exists(skseFont)) {
		return skseFont;
	}
	return "C:\\Windows\\Fonts\\msyh.ttc";
}

void PutStyles(ImGuiStyle& a_pStyle)
{
	a_pStyle.WindowBorderSize = 0.f;
	a_pStyle.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	a_pStyle.WindowRounding = 7.f;
}

#pragma region Hooks_Impl
void Hooks::RendererManager::Hook_InitD3D::hooked()
{
	INFO("Calling origin InitD3D");
	oldFunc();
	auto pRendererManager = RE::BSGraphics::Renderer::GetSingleton();
	if (!pRendererManager) {
		ERROR("[Renderer] Failed get rendererManager");
	}

	auto renderData = pRendererManager->data;
	auto pSwapChain = renderData.renderWindows->swapChain;
	auto pDevice = renderData.forwarder;
	auto pDeviceContext = renderData.context;
	if (!pSwapChain) {
		ERROR("[Renderer] Failed get SwapChain");
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	if (FAILED(pSwapChain->GetDesc(&desc))) {
		ERROR("[Renderer] Failed get DXGI_SWAP_CHAIN_DESC");
	}
	IMEPanel::GetSingleton()->Initialize(desc.OutputWindow);

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	PutStyles(ImGui::GetStyle());

	static ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder glyphBuilder;
	glyphBuilder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());

	// Read glyph range from fontconfig.txt
	INFO("Reading Glyph Range from {}", Configs::sGlyphRangeSourcePath.data());
	FILE* inFile = fopen(Configs::sGlyphRangeSourcePath.data(), "rb");
	if (inFile == 0) {
		INFO("Can't read {}", Configs::sGlyphRangeSourcePath.data());
	} else {
		unsigned char* charBuf;
		fseek(inFile, 0, SEEK_END);
		int fileLen = ftell(inFile);
		charBuf = new unsigned char[fileLen];
		fseek(inFile, 0, SEEK_SET);
		fread(charBuf, fileLen, 1, inFile);
		fclose(inFile);
		glyphBuilder.AddText((const char*)charBuf);
		delete[] charBuf;
	}
	glyphBuilder.BuildRanges(&ranges);
	io.Fonts->AddFontFromFileTTF(SafeGetFont(Configs::sFontPath).c_str(), Configs::fFontSize, NULL, ranges.Data);
	io.Fonts->Build();

	if (!ImGui_ImplDX11_Init(pDevice, pDeviceContext)) {
		ERROR("[RendererManager] Failed init imgui (DX11)")
	}
	if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
		ERROR("[RendererManager] Failed init imgui (Win32)")
	}
	RendererManager::GetSingleton()->m_bInitialized.store(true);

	Hooks::WindowsManager::GetSingleton()->Install(desc.OutputWindow);
}

void Hooks::RendererManager::Hook_D3D_Present::hooked(std::uint32_t a_p1)
{
	oldFunc(a_p1);
	if (!RendererManager::GetSingleton()->m_bInitialized.load()) {
		return;
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	IMEPanel::GetSingleton()->OnRender();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
#pragma endregion

namespace Hooks
{
	void RendererManager::Install()
	{
		INFO("Installing RendererManager");

		INFO("Installing InitD3D Hook");
		Hook_InitD3D::Install();

		INFO("Installing Present Hook");
		Hook_D3D_Present::Install();
	}
}