#include "Utils.h"

#include "Helpers/DebugHelper.h"

#include "Config.h"
#include "InGameIme.h"
#include "RE/CustomRE.h"
#include "RingBuffer.h"

#define BUFFER_SIZE 400

namespace Utils
{
	GFxCharEvent* char_buffer = (GFxCharEvent*)malloc(sizeof(GFxCharEvent) * BUFFER_SIZE);
	int buffer_position = 0;

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

	std::string ConvertToANSI(std::string s, UINT sourceCodePage)
	{
		BSTR bstrWide;
		char* pszAnsi;
		int nLength;
		const char* pszCode = s.c_str();

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
		ERROR("Failed Allocate Heap");
		return nullptr;
	}

	void HeapFree(void* ptr)
	{
		RE::MemoryManager* pMemoryManager = RE::MemoryManager::GetSingleton();
		if (pMemoryManager) {
			pMemoryManager->Deallocate(ptr, false);
			return;
		}
		ERROR("Failed Free Heap");
	}

	bool SendUnicodeMessage(UINT32 code)
	{
		DH_DEBUGW(L"Utils::SendUnicodeMessage Sending {}", code);
		if (code == 96 || code == 183) {
			return false;
		}

		RE::InterfaceStrings* interface_strings = RE::InterfaceStrings::GetSingleton();
		RE::ControlMap* control_map = RE::ControlMap::GetSingleton();

		if (control_map->textEntryCount) {
			auto pUIMessageQueue = RE::UIMessageQueue::GetSingleton();
			auto pFactoryManager = RE::MessageDataFactoryManager::GetSingleton();
			auto pFactory = pFactoryManager->GetCreator<RE::BSUIScaleformData>(interface_strings->bsUIScaleformData);
			auto scaleFormMessageData = pFactory ? pFactory->Create() : nullptr;

			if (scaleFormMessageData == nullptr) {
				ERROR("Cast Pointer Data Failed");
				return false;
			}
			GFxCharEvent* charEvent = new GFxCharEvent(code, 0);
			scaleFormMessageData->scaleformEvent = charEvent;
			RE::BSFixedString menuName = interface_strings->topMenu;
			pUIMessageQueue->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kScaleformEvent, scaleFormMessageData);
			buffer_position = (buffer_position + 1) % BUFFER_SIZE;
			return true;
		}
		DH_DEBUG("= END FUNC = Utils::SendUnicodeMessage");
		return false;
	}

	void GetCandidateList(HWND hWnd)
	{
		DH_DEBUG("= FUNC = Utils::GetCandidateList");
		InGameIME* ime = InGameIME::GetSingleton();
		if (!InterlockedCompareExchange(&ime->enableState, ime->enableState, 2)) {
			return;
		}
		HIMC hIMC = ImmGetContext(hWnd);
		if (hIMC == nullptr) {
			return;
		}
		DWORD dwBufLen = ImmGetCandidateList(hIMC, 0, nullptr, 0);
		if (!dwBufLen) {
			ImmReleaseContext(hWnd, hIMC);
			return;
		}
		std::unique_ptr<CANDIDATELIST, void(__cdecl*)(void*)> pList(
			reinterpret_cast<LPCANDIDATELIST>(HeapAlloc(dwBufLen)), HeapFree);
		if (!pList) {
			ImmReleaseContext(hWnd, hIMC);
			return;
		}
		ImmGetCandidateList(hIMC, 0, pList.get(), dwBufLen);
		if (pList->dwStyle != IME_CAND_CODE) {
			WCHAR buffer[8];

			InterlockedExchange(&ime->selectedIndex, pList->dwSelection);
			InterlockedExchange(&ime->pageStartIndex, pList->dwPageStart);

			ime_critical_section.Enter();

			ime->candidateList.clear();
			// const Configs::IME &imeConfig = Config::GetSingleton().GetIme();

			for (unsigned int i = 0; i < pList->dwCount && i < pList->dwPageSize && i < 0; i++) {
				wprintf_s(buffer, L"%d.", i + 1);
				WCHAR* pSubStr = (WCHAR*)((BYTE*)pList.get() + pList->dwOffset[i + pList->dwPageStart]);
				std::wstring temp(buffer);
				temp += pSubStr;
				ime->candidateList.push_back(temp);
			}

			ime_critical_section.Leave();
		}
		ImmReleaseContext(hWnd, hIMC);
		DH_DEBUG("= END FUNC = Utils::GetCandidateList");
	}

	void GetResultString(const HWND& hWnd)
	{
		DH_DEBUG("= FUNC = Utils::GetResultString");
		HIMC hImc = ImmGetContext(hWnd);

		if (hImc) {
			DWORD bufferSize = ImmGetCompositionString(hImc, GCS_RESULTSTR, nullptr, 0);
			if (bufferSize) {
				bufferSize += sizeof(WCHAR);
				std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> wCharBuffer(
					reinterpret_cast<WCHAR*>(HeapAlloc(bufferSize)), HeapFree);
				ZeroMemory(wCharBuffer.get(), bufferSize);
				ImmGetCompositionString(hImc, GCS_RESULTSTR, wCharBuffer.get(), bufferSize);
				size_t len = bufferSize / sizeof(WCHAR);

				for (size_t i = 0; i < len; i++) {
					WCHAR unicode = wCharBuffer[i];
					if (unicode > 0) {
						SendUnicodeMessage(unicode);
					}
				}
			}
		}
		ImmReleaseContext(hWnd, hImc);
		DH_DEBUG("= END FUNC = Utils::GetResultString");
	}

	void GetInputString(const HWND& hWnd)
	{
		DH_DEBUG("= FUNC = Utils::GetInputString");
		HIMC hImc = ImmGetContext(hWnd);
		UINT uLen;
		uLen = ImmGetCompositionString(hImc, GCS_COMPSTR, NULL, 0);
		DH_DEBUG("Composition String's Length: {}", uLen);
		if (uLen) {
			DH_DEBUG("Allocating Memory of szCompStr");
			UINT uMem = uLen + 1;
			std::unique_ptr<WCHAR[], void(__cdecl*)(void*)> szCompStr(
				reinterpret_cast<WCHAR*>(HeapAlloc(uMem * sizeof(WCHAR))),
				HeapFree);

			if (szCompStr) {
				szCompStr[uLen] = 0;
				ImmGetCompositionString(hImc, GCS_COMPSTR, szCompStr.get(), uMem);
				std::wcout << L"szCompStr:" << szCompStr << std::endl;
				std::wcout << L"Text" << std::endl;

				InGameIME* ime = InGameIME::GetSingleton();

				ime_critical_section.Enter();
				ime->inputContent = std::wstring(szCompStr.get());
				DH_DEBUGW(L"Input Content: {}", ime->inputContent);
				ime_critical_section.Leave();
			} else {
				DH_DEBUG("Allocation of szCompStr Failed");
			}
		}
		ImmReleaseContext(hWnd, hImc);
		DH_DEBUG("= END FUNC = Utils::GetInputString");
	}
}