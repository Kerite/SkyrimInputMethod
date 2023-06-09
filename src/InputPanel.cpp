#include "InputPanel.h"

#include "Cirero.h"
#include "Config.h"
#include "Utils.h"

void IMEPanel::Initialize(HWND hWnd)
{
	hWindow = hWnd;

	ImmAssociateContextEx(hWnd, NULL, 0);
}

void IMEPanel::OnRender()
{
	static constexpr ImGuiWindowFlags imguiFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs;

	if (RE::ControlMap::GetSingleton()->textEntryCount) {
		std::uint32_t iSelectedIndex = m_ulSlectedIndex.load();
		std::uint32_t iPageStartIndex = m_ulPageStartIndex.load();

		ImGui::SetNextWindowPos(ImVec2(Configs::fPositionX, Configs::fPositionY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(250, 0.f), ImGuiCond_Always);

		if (ImGui::Begin(Utils::utf8_encode(wstrInputMethodName).c_str(), nullptr, imguiFlag)) {
			ImGui::Text(Utils::utf8_encode(wstrComposition).c_str());
			int highlighted = iSelectedIndex - iPageStartIndex;  // Index of current highlighted candidate
			for (size_t i = 0; i < vwsCandidateList.size(); i++) {
				ImGui::Selectable(Utils::utf8_encode(vwsCandidateList[i]).c_str(), i == highlighted, 0, ImVec2(0.f, 0.f));
			}
		}
		ImGui::End();
	}
}