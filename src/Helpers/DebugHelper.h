#pragma once

#include "Utils.h"

#define DH_DEBUG(...)                                              \
	{                                                              \
		std::string dh_message = fmt::format(__VA_ARGS__);         \
		DebugHelper::GetSingleton()->SendDebugMessage(dh_message); \
	}

#define DH_DEBUGW(...)                                               \
	{                                                                \
		std::wstring dh_message = fmt::format(__VA_ARGS__);          \
		std::string message = Utils::WideStringToString(dh_message); \
		DebugHelper::GetSingleton()->SendDebugMessage(message);      \
	}

#define DH_INFO(...)                                              \
	{                                                             \
		std::string dh_message = fmt::format(__VA_ARGS__);        \
		DebugHelper::GetSingleton()->SendInfoMessage(dh_message); \
	}

#define DH_INFOW(...)                                                \
	{                                                                \
		std::wstring dh_message = fmt::format(__VA_ARGS__);          \
		std::string message = Utils::WideStringToString(dh_message); \
		DebugHelper::GetSingleton()->SendInfoMessage(message);       \
	}

class DebugHelper final : public Singleton<DebugHelper>
{
public:
	void Install();

	void SendDebugMessage(std::string message);
	void SendInfoMessage(std::string message);

	~DebugHelper();

private:
	FILE* fp;
	HANDLE hPipe;
	DWORD dwWritten;

	BOOL m_bInstalled;
};