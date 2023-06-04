#pragma once

class GFxCharEvent : public RE::GFxEvent
{
public:
	GFxCharEvent() = default;
	GFxCharEvent(UINT32 a_wcharCode, UINT8 a_keyboardIndex = 0) :
		GFxEvent(RE::GFxEvent::EventType::kCharEvent), wcharCode(a_wcharCode), keyboardIndex(a_keyboardIndex)
	{
	}
	// @members
	std::uint32_t wcharCode;     // 04
	std::uint32_t keyboardIndex;  // 08
};
static_assert(sizeof(GFxCharEvent) == 0x0C);
