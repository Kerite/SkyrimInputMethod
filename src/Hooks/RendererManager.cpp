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
	a_pStyle.WindowRounding = 10.f;
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
	// Read glyph range from fontconfig.txt
	FILE* inFile = fopen("Data\\Interface\\fontconfig.txt", "rb");
	if (inFile == 0) {
		ERROR("Can't read Data\\Interface\\fontconfig.txt");
	}
	unsigned char* charBuf;
	fseek(inFile, 0, SEEK_END);
	int fileLen = ftell(inFile);
	charBuf = new unsigned char[fileLen];
	fseek(inFile, 0, SEEK_SET);
	fread(charBuf, fileLen, 1, inFile);
	fclose(inFile);
	static ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder glyphBuilder;
	glyphBuilder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
	glyphBuilder.AddText((const char*)charBuf);
	glyphBuilder.BuildRanges(&ranges);
	auto pConfig = Configs::GetSingleton();
	io.Fonts->AddFontFromFileTTF(SafeGetFont(Configs::sFontPath).c_str(), Configs::fFontSize, NULL, ranges.Data);
	io.Fonts->Build();
	delete[] charBuf;

	if (!ImGui_ImplDX11_Init(pDevice, pDeviceContext)) {
		ERROR("[Renderer] Failed init imgui (DX11)")
	}
	if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
		ERROR("[Renderer] Failed init imgui (Win32)")
	}
	RendererManager::GetSingleton()->m_bInitialized.store(true);

	Hooks::WindowsManager::GetSingleton()->Install(desc.OutputWindow);
}

void Hooks::RendererManager::Hook_Present::hooked(std::uint32_t a_p1)
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
		Utils::Hook::WriteCall<Hook_InitD3D>();

		INFO("Installing Present Hook");
		Utils::Hook::WriteCall<Hook_Present>();
	}
}