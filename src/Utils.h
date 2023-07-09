#pragma once

#define DISABLE_IME(hwnd) PostMessage(hwnd, WM_IME_SETSTATE, NULL, 0)
#define ENABLE_IME(hwnd) PostMessage(hwnd, WM_IME_SETSTATE, NULL, 1)

template <class T>
void SafeRelease(T** ppT)
{
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

namespace Utils
{
	std::string WideStringToString(const std::wstring wstr);

	/// Convert Utf-8 string to ANSI string
	std::string ConvertToANSI(std::string s, UINT sourceCodePage = CP_UTF8);

	void ConvertCodeSet(const char* a_strIn, char* a_strOut, int a_sourceCodepage, int a_targetCodepage);

	void* HeapAlloc(size_t size);

	void HeapFree(void* ptr);

	/// Send scaleform char message to game
	/// ����Scaleform�ַ���Ϣ����Ϸ
	bool SendUnicodeMessage(UINT32 code);

	/// Update candidate list from IMM
	/// ���º�ѡ���б�IMM�ӿڣ�
	void UpdateCandidateList(HWND hWnd);

	/// Update inputed content from IMM
	/// �����������ݣ�IMM�ӿڣ�
	void UpdateInputContent(const HWND& hWnd);

	void UpdateInputMethodName(HKL hkl);

	/// Get result string and send it to game
	/// ��ȡ������ҷ��͵���Ϸ
	void GetResultString(const HWND& hWnd);

	void GetClipboard();

	// Convert a wide Unicode string to an UTF8 string
	std::string utf8_encode(const std::wstring& wstr);
	// Convert an UTF8 string to a wide Unicode String
	std::wstring utf8_decode(const std::string& str);
	// Convert an wide Unicode string to ANSI string
	std::string unicode2ansi(const std::wstring& wstr);
	// Convert an ANSI string to a wide Unicode String
	std::wstring ansi2unicode(const std::string& str);
}