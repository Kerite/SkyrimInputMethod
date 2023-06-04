#pragma once

#include "DKUtil/Config.hpp"

class Configs : public Singleton<Configs>
{
public:
	dku::Alias::Integer mCandidateSize{ "CandidateSize" };
	void Load() noexcept;
};