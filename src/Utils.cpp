#include "Utils.h"

#include "Helpers/DebugHelper.h"

#include "Cirero.h"
#include "Config.h"
#include "InputPanel.h"
#include "RE/CustomRE.h"
#include "ScaleformManager.h"

#define BUFFER_SIZE 400

namespace Utils
{
	std::string WideStringToString(const std::wstring wstr)
	{
		LPSTR buffer = nullptr;
		int targetLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer, 0, NULL, NULL);
		buffer = new CHAR[targetLen + 1];
		buffer[targetLen] = 0;
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer, targetLen, NULL, NULL);
		std::string result(buffer);
		delete[] buffer;
		return result;
	}

	std::string ConvertToANSI(std::string str, UINT sourceCodePage)
	{
		BSTR bstrWide;
		char* pszAnsi;
		int nLength;
		const char* pszCode = str.c_str();

		nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
		bstrWide = SysAllocStringLen(NULL, nLength);

		MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

		nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
		pszAnsi = new char[nLength];

		WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
		SysFreeString(bstrWide);

		std::string r(pszAnsi);
		delete[] pszAnsi;
		return r;
	}

	void ConvertCodeSet(const char* strIn, char* strOut, int sourceCodepage, int targetCodepage)
	{
		int len = strlen(strIn);
		int unicodeLen = MultiByteToWideChar(sourceCodepage, 0, strIn, -1, NULL, 0);
		WCHAR* pUnicode = new WCHAR[unicodeLen + 1];
		memset(pUnicode, 0, sizeof(WCHAR) * (unicodeLen + 1));
		MultiByteToWideChar(targetCodepage, 0, strIn, -1, (LPWSTR)pUnicode, unicodeLen);
		BYTE* pTargetData = nullptr;
		int targetLen = WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode, -1, (char*)pTargetData, 0, NULL, NULL);
		pTargetData = new BYTE[targetLen + 1];
		memset(pTargetData, 0, targetLen + 1);
		WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode, -1, (char*)pTargetData, targetLen, NULL, NULL);
		strcpy(strOut, (char*)pTargetData);
		delete[] pUnicode;
		delete[] pTargetData;
	}

	void* HeapAlloc(std::size_t size)
	{
		RE::MemoryManager* pMemoryManager = RE::MemoryManager::GetSingleton();
		if (pMemoryManager) {
			return pMemoryManager->Allocate(size, 0, false);
		} else {
			spdlog::error("Failed alloc heap");
		}
		return nullptr;
	}

	void HeapFree(void* ptr)
	{
		RE::MemoryManager* pMemoryManager = RE::MemoryManager::GetSingleton();
		if (pMemoryManager) {
			pMemoryManager->Deallocate(ptr, false);
		} else {
			spdlog::error("Failed free heap");
		}
	}

	void UpdateCandidateList(HWND hWnd)
	{
		if (Cicero::GetSingleton()->bCiceroState) {
			// When TFS is enabled, candidate list is fetched from TFS
			return;
		}
		DEBUG("[Utils::UpdateCandidateList] == Start ==");
		IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
		HIMC hIMC = ImmGetContext(hWnd);
		if (hIMC == nullptr) {
			DEBUG("[Utils::UpdateCandidateList] == Finish == Context is null, cancel")
			return;
		}
		DWORD dwBufLen = ImmGetCandidateList(hIMC, 0, nullptr, 0);  // Get size first
		if (!dwBufLen) {
			ImmReleaseContext(hWnd, hIMC);
			DEBUG("[Utils::UpdateCandidateList] == Finish == Candidate list is empty, cancel")
			return;
		}
		std::unique_ptr<CANDIDATELIST, void(__cdecl*)(void*)> pCandidateList(
			reinterpret_cast<LPCANDIDATELIST>(HeapAlloc(dwBufLen)), HeapFree);
		if (!pCandidateList) {
			ImmReleaseContext(hWnd, hIMC);
			DEBUG("[Utils::UpdateCandidateList] == Finish == Alocation memory is empty, cancel")
			return;
		}
		ImmGetCandidateList(hIMC, 0, pCandidateList.get(), dwBufLen);
		DEBUG("CandidateList.dwStyle: {}", pCandidateList->dwStyle);
		if (pCandidateList->dwStyle != IME_CAND_CODE) {
			DEBUG("CandidateList dwCount: {}, dwSize: {}", pCandidateList->dwCount, pCandidateList->dwSize);
			DEBUG("CandidateList dwSelection: {}, dwPageStart: {}", pCandidateList->dwSelection, pCandidateList->dwPageStart);

			InterlockedExchange(&pIMEPanel->ulSlectedIndex, pCandidateList->dwSelection);
			InterlockedExchange(&pIMEPanel->ulPageStartIndex, pCandidateList->dwPageStart);

			WCHAR buffer[MAX_PATH];

			pIMEPanel->csImeInformation.Enter();
			pIMEPanel->vwsCandidateList.clear();
			for (int i = 0; i < pCandidateList->dwCount && i < pCandidateList->dwPageSize && i < Configs::iCandidateSize; i++) {
				wsprintf(buffer, L"%d.", i + 1);
				WCHAR* pSubStr = (WCHAR*)((BYTE*)pCandidateList.get() + pCandidateList->dwOffset[i + pCandidateList->dwPageStart]);
				std::wstring temp(buffer);
				temp += pSubStr;
				pIMEPanel->vwsCandidateList.push_back(temp);
				DEBUG("(IMM) {}", Utils::WideStringToString(temp));
			}
			pIMEPanel->csImeInformation.Leave();
		}
		ImmReleaseContext(hWnd, hIMC);
		DEBUG("[Utils::UpdateCandidateList] == Finish ==\n");
	}

	void UpdateInputContent(const HWND& hWnd)
	{
		if (Cicero::GetSingleton()->bCiceroState) {
			// When TFS is enabled, candidate list is fetched from TFS
			return;
		}
		IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
		HIMC hImc = ImmGetContext(hWnd);
		LONG uNeededBytes, uStrLen;
		DEBUG("= START FUNC = Utils::UpdateInputContent");
		uNeededBytes = ImmGetCompositionString(hImc, GCS_COMPSTR, NULL, 0);
		if (!uNeededBytes) {
			DEBUG("Composition String is empty, cancel");
			ImmReleaseContext(hWnd, hImc);
			return;
		}
		uStrLen = (uNeededBytes / 2) + 1;
		std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> szCompStr(
			reinterpret_cast<WCHAR*>(HeapAlloc(uStrLen * sizeof(WCHAR))),
			HeapFree);

		if (szCompStr) {
			LONG writedBytes = ImmGetCompositionString(hImc, GCS_COMPSTR, szCompStr.get(), uNeededBytes);
			szCompStr[uStrLen - 1] = '\0';

			pIMEPanel->csImeInformation.Enter();
			pIMEPanel->wstrComposition = std::wstring(szCompStr.get());
			DEBUG("(IMM) Update InputContent: {}, Length: {}", Utils::WideStringToString(pIMEPanel->wstrComposition), pIMEPanel->wstrComposition.size());
			pIMEPanel->csImeInformation.Leave();
		} else {
			DEBUG("Allocation of szCompStr Failed");
		}
		ImmReleaseContext(hWnd, hImc);
		DEBUG("= END FUNC = Utils::UpdateInputContent");
	}

	void GetResultString(const HWND& hWnd)
	{
		DEBUG("[Utils::GetResultString] == Start ==");
		HIMC hImc = ImmGetContext(hWnd);
		if (!hImc) {
			DEBUG("[Utils::GetResultString] Context is empty, cancel\n");
			return;
		}
		DWORD bufferSize = ImmGetCompositionString(hImc, GCS_RESULTSTR, nullptr, 0);
		if (!bufferSize) {
			DEBUG("[Utils::GetResultString] Composition is empty, cancel\n");
			ImmReleaseContext(hWnd, hImc);
			return;
		}
		bufferSize += sizeof(WCHAR);
		std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> pBuffer(
			reinterpret_cast<WCHAR*>(HeapAlloc(bufferSize)), HeapFree);
		if (!pBuffer) {
			DEBUG("[Utils::GetResultString] Composition is empty, cancel\n");
			ImmReleaseContext(hWnd, hImc);
			return;
		}
		ZeroMemory(pBuffer.get(), bufferSize);
		ImmGetCompositionString(hImc, GCS_RESULTSTR, pBuffer.get(), bufferSize);
		size_t len = bufferSize / sizeof(WCHAR);

		for (size_t i = 0; i < len; i++) {
			WCHAR unicode = pBuffer[i];
			if (unicode > 0) {
				SendUnicodeMessage(unicode);
			}
		}
		ImmReleaseContext(hWnd, hImc);
		DEBUG("[Utils::GetResultString] == Finish ==\n");
	}

	void UpdateInputMethodName(HKL hkl)
	{
		if (Cicero::GetSingleton()->bCOMInitialized) {
			return;
		}
		WCHAR buffer[MAX_PATH];
		ZeroMemory(buffer, MAX_PATH);
		UINT uByteLength = ImmGetIMEFileNameW(hkl, NULL, 0);
		ImmGetIMEFileNameW(hkl, buffer, uByteLength);
		IMEPanel::GetSingleton()->wstrInputMethodName = std::wstring(buffer);
		DEBUG("[Utils::UpdateInputMethodName] Method Name: {}", Utils::WideStringToString(IMEPanel::GetSingleton()->wstrInputMethodName));
	}

	bool SendUnicodeMessage(UINT32 code)
	{
		// Ignore '`' and '¡¤', for console
		if (code == 96 || code == 183) {
			return false;
		}

		if (!RE::ControlMap::GetSingleton()->textEntryCount) {  // Don't send unicode if no text input box is opened
			return false;
		}

		auto pInterfaceStrings = RE::InterfaceStrings::GetSingleton();

		auto pFactoryManager = RE::MessageDataFactoryManager::GetSingleton();
		auto pFactory = pFactoryManager->GetCreator<RE::BSUIScaleformData>(pInterfaceStrings->bsUIScaleformData);
		auto pScaleFormMessageData = pFactory ? pFactory->Create() : nullptr;

		if (pScaleFormMessageData == nullptr) {
			ERROR("Cast Pointer Data Failed");
			return false;
		}
		// Start send message
		GFxCharEvent* pCharEvent = new GFxCharEvent(code, 0);
		pScaleFormMessageData->scaleformEvent = pCharEvent;
		RE::BSFixedString menuName = pInterfaceStrings->topMenu;

		DEBUG("(Utils::SendUnicodeMessage) Sending {}", code);

		RE::UIMessageQueue::GetSingleton()->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kScaleformEvent, pScaleFormMessageData);
		return true;
	}

	void GetClipboard()
	{
		DEBUG("Pasting text");
		Configs* pConfigs = Configs::GetSingleton();
		ScaleformManager* pScaleformManager = ScaleformManager::GetSingleton();
		if (!pConfigs->bAllowPasteInConsole && InterlockedCompareExchange(&pScaleformManager->bConsoleOpenState, pScaleformManager->bConsoleOpenState, 0xFF)) {
			DEBUG("Console is opened and another copy-paste mod installed, cancel");
			return;
		}

		if (!OpenClipboard(nullptr))
			return;
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (!hData)
			return;
		LPCWSTR pszText = static_cast<LPCWSTR>(GlobalLock(hData));
		int lTextLength = lstrlen(pszText);
		if (pszText == nullptr)
			return;
		for (int i = 0; i < lTextLength; i++) {
			SendUnicodeMessage(pszText[i]);
		}
		GlobalUnlock(hData);
		CloseClipboard();
	}

	// Convert a wide Unicode string to an UTF8 string
	std::string utf8_encode(const std::wstring& wstr)
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	// Convert an UTF8 string to a wide Unicode String
	std::wstring utf8_decode(const std::string& str)
	{
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	// Convert an wide Unicode string to ANSI string
	std::string unicode2ansi(const std::wstring& wstr)
	{
		int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	// Convert an ANSI string to a wide Unicode String
	std::wstring ansi2unicode(const std::string& str)
	{
		int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
}