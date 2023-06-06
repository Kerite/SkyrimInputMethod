#pragma once

class CCriticalSection
{
public:
	CCriticalSection() { InitializeCriticalSection(&critSection); }
	~CCriticalSection() { DeleteCriticalSection(&critSection); }

	void Enter(void) { EnterCriticalSection(&critSection); }
	void Leave(void) { LeaveCriticalSection(&critSection); }
	bool TryEnter(void) { return TryEnterCriticalSection(&critSection) != 0; }

private:
	RTL_CRITICAL_SECTION critSection;
};