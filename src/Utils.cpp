#include "Utils.h"

#include "Helpers/DebugHelper.h"

#include "Cirero.h"
#include "Config.h"
#include "InGameIme.h"
#include "RE/CustomRE.h"

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
		}
		return nullptr;
	}

	void HeapFree(void* ptr)
	{
		RE::MemoryManager* pMemoryManager = RE::MemoryManager::GetSingleton();
		if (pMemoryManager) {
			pMemoryManager->Deallocate(ptr, false);
			spdlog::error("Failed free heap");
		}
	}

	void UpdateCandidateList(HWND hWnd)
	{
		if (Cicero::GetSingleton()->ciceroState) {
			// When TFS is enabled, candidate list is fetched from TFS
			return;
		}
		DH_DEBUG("[Utils::UpdateCandidateList] == Start ==");
		InGameIME* pInGameIME = InGameIME::GetSingleton();
		HIMC hIMC = ImmGetContext(hWnd);
		if (hIMC == nullptr) {
			DH_DEBUG("[Utils::UpdateCandidateList] == Finish == Context is null, cancel")
			return;
		}
		DWORD dwBufLen = ImmGetCandidateList(hIMC, 0, nullptr, 0);
		if (!dwBufLen) {
			ImmReleaseContext(hWnd, hIMC);
			DH_DEBUG("[Utils::UpdateCandidateList] == Finish == Candidate list is empty, cancel")
			return;
		}
		std::unique_ptr<CANDIDATELIST, void(__cdecl*)(void*)> pList(
			reinterpret_cast<LPCANDIDATELIST>(HeapAlloc(dwBufLen)), HeapFree);
		if (!pList) {
			ImmReleaseContext(hWnd, hIMC);
			DH_DEBUG("[Utils::UpdateCandidateList] == Finish == Alocation memory is empty, cancel")
			return;
		}
		ImmGetCandidateList(hIMC, 0, pList.get(), dwBufLen);
		DH_DEBUG("Candidate: {}", pList->dwStyle);
		if (pList->dwStyle != IME_CAND_CODE) {
			WCHAR buffer[8];

			InterlockedExchange(&pInGameIME->selectedIndex, pList->dwSelection);
			InterlockedExchange(&pInGameIME->pageStartIndex, pList->dwPageStart);

			pInGameIME->imeCriticalSection.Enter();
			pInGameIME->candidateList.clear();
			for (int i = 0; i < pList->dwCount && i < pList->dwPageSize && i < 0; i++) {
				wprintf_s(buffer, L"%d.", i + 1);
				WCHAR* pSubStr = (WCHAR*)((BYTE*)pList.get() + pList->dwOffset[i + pList->dwPageStart]);
				std::wstring temp(buffer);
				temp += pSubStr;
				pInGameIME->candidateList.push_back(temp);
				DH_DEBUGW(L"(IMM) {}", temp);
			}
			pInGameIME->imeCriticalSection.Leave();
		}
		ImmReleaseContext(hWnd, hIMC);
		DH_DEBUG("[Utils::UpdateCandidateList] == Finish ==\n");
	}

	void UpdateInputContent(const HWND& hWnd)
	{
		if (Cicero::GetSingleton()->ciceroState) {
			// When TFS is enabled, candidate list is fetched from TFS
			return;
		}
		InGameIME* pIme = InGameIME::GetSingleton();
		HIMC hImc = ImmGetContext(hWnd);
		LONG uNeededBytes, uStrLen;
		DH_DEBUG("= START FUNC = Utils::UpdateInputContent");
		uNeededBytes = ImmGetCompositionString(hImc, GCS_COMPSTR, NULL, 0);
		if (!uNeededBytes) {
			DH_DEBUG("Composition String is empty, cancel");
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

			pIme->imeCriticalSection.Enter();
			pIme->inputContent = std::wstring(szCompStr.get());
			DH_DEBUGW(L"(IMM) Update InputContent: {}, Length: {}", pIme->inputContent, pIme->inputContent.size());
			pIme->imeCriticalSection.Leave();
		} else {
			DH_DEBUG("Allocation of szCompStr Failed");
		}
		ImmReleaseContext(hWnd, hImc);
		DH_DEBUG("= END FUNC = Utils::UpdateInputContent");
	}

	void GetResultString(const HWND& hWnd)
	{
		DH_DEBUG("[Utils::GetResultString] == Start ==");
		HIMC hImc = ImmGetContext(hWnd);
		if (!hImc) {
			DH_DEBUG("[Utils::GetResultString] Context is empty, cancel\n");
			return;
		}
		DWORD bufferSize = ImmGetCompositionString(hImc, GCS_RESULTSTR, nullptr, 0);
		if (!bufferSize) {
			DH_DEBUG("[Utils::GetResultString] Composition is empty, cancel\n");
			ImmReleaseContext(hWnd, hImc);
			return;
		}
		bufferSize += sizeof(WCHAR);
		std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> pBuffer(
			reinterpret_cast<WCHAR*>(HeapAlloc(bufferSize)), HeapFree);
		if (!pBuffer) {
			DH_DEBUG("[Utils::GetResultString] Composition is empty, cancel\n");
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
		DH_DEBUG("[Utils::GetResultString] == Finish ==\n");
	}

	bool SendUnicodeMessage(UINT32 code)
	{
		// Ignore '`' and '¡¤'
		if (code == '`' || code == '¡¤') {
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

		DH_DEBUGW(L"(Utils::SendUnicodeMessage) Sending {}", code);

		RE::UIMessageQueue::GetSingleton()->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kScaleformEvent, pScaleFormMessageData);
		return true;
	}
}