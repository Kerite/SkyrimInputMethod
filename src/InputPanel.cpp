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
		std::uint32_t iSelectedIndex = InterlockedCompareExchange(&ulSlectedIndex, ulSlectedIndex, -1);
		std::uint32_t iPageStartIndex = InterlockedCompareExchange(&ulPageStartIndex, ulPageStartIndex, -1);

		ImGui::SetNextWindowPos(ImVec2(Configs::fPositionX, Configs::fPositionY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(250, 0.f), ImGuiCond_Always);

		ImGui::Begin(Utils::utf8_encode(wstrInputMethodName).c_str(), nullptr, imguiFlag);
		ImGui::Text(Utils::utf8_encode(wstrComposition).c_str());
		int currentHighlighted = iSelectedIndex - iPageStartIndex;
		for (size_t i = 0; i < vwsCandidateList.size(); i++) {
			bool highlighted = i == currentHighlighted;
			ImGui::Selectable(Utils::utf8_encode(vwsCandidateList[i]).c_str(), highlighted, 0, ImVec2(0.f, 0.f));
		}
		ImGui::End();
	}
}