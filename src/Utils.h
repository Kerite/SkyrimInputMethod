#pragma once
#include "detours/detours.h"

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

	template <class T>
	void DetourAttach(REL::ID id)
	{
		T::oldFunc = REL::Relocation<decltype(&T::hooked)>(id).get();
		::DetourAttach(reinterpret_cast<void**>(&T::oldFunc), reinterpret_cast<void*>(T::hooked));
	}

	template <class T>
	void WriteCall(std::uintptr_t a_address)
	{
		SKSE::AllocTrampoline(14);
		auto& trampoline = SKSE::GetTrampoline();
		T::oldFunc = trampoline.write_call<5>(a_address, T::hooked);
	}

	/// Convert Utf-8 string to ANSI string
	std::string ConvertToANSI(std::string s, UINT sourceCodePage = CP_UTF8);

	void ConvertCodeSet(const char* a_strIn, char* a_strOut, int a_sourceCodepage, int a_targetCodepage);

	void* HeapAlloc(size_t size);

	void HeapFree(void* ptr);

	bool SendUnicodeMessage(UINT32 code);

	void UpdateCandidateList(HWND hWnd);

	void GetResultString(const HWND& hWnd);

	void GetInputString(const HWND& hWnd);
}