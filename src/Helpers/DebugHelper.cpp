#include "Helpers/DebugHelper.h"

void DebugHelper::SendDebugMessage(std::string message)
{
	DEBUG(message);
	if (!m_bInstalled) {
		return;
	}
	if (hPipe != INVALID_HANDLE_VALUE && m_bInstalled) {
		WriteFile(hPipe, message.c_str(), message.size() + 1, &dwWritten, NULL);
	}
	std::cout << "[DEBUG] " << message << std::endl;
}

void DebugHelper::SendInfoMessage(std::string message)
{
	INFO(message);
	if (!m_bInstalled) {
		return;
	}
	if (hPipe != INVALID_HANDLE_VALUE && m_bInstalled) {
		WriteFile(hPipe, message.c_str(), message.size() + 1, &dwWritten, NULL);
	}
	std::cout << "[INFO ] " << message << std::endl;
}

void DebugHelper::Install()
{
	//AllocConsole();
	//freopen_s(&fp, "CONIN$", "r", stdin);
	//freopen_s(&fp, "CONOUT$", "w", stdout);
	//freopen_s(&fp, "CONOUT$", "w", stderr);
	//m_bInstalled = true;
	//hPipe = CreateFile(TEXT("\\\\.\\pipe\\skyrim_development"),
	//	GENERIC_READ | GENERIC_WRITE,
	//	0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

DebugHelper::~DebugHelper()
{
	CloseHandle(hPipe);
}