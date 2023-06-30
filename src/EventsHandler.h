#pragma once

class EventsHandler :
	public Singleton<EventsHandler>,
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	void Install();

	void CopyText();

	volatile ULONG bConsoleOpenState;

private:
	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent*, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	volatile bool m_bInitialized = false;
};