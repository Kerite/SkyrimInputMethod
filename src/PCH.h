#pragma once

// c
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <cwctype>

// cxx
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <barrier>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <compare>
#include <complex>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <latch>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <semaphore>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <syncstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include <version>

// clib
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

// winnt
#include <ShlObj_core.h>

#include <fmt/xchar.h>

using namespace std::literals;
using namespace REL::literals;

#define DLLEXPORT extern "C" [[maybe_unused]] __declspec(dllexport)

// Plugin
#include "Plugin.h"

// DKUtil
#include "DKUtil/Extra.hpp"
#include "DKUtil/Hook.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"

using namespace dku::model;

#define HOOK_MAKE_THIS(hookName, retName, thisName, ...)          \
	struct Hook_##hookName                                        \
	{                                                             \
		static retName __fastcall hooked(thisName*, __VA_ARGS__); \
		static inline REL::Relocation<decltype(hooked)> oldFunc;  \
	};
#define HOOK_FUNC_THIS(managerName, hookName, retName, thisName, ...) \
	retName __fastcall managerName::Hook_##hookName::hooked(thisName* a_pThis, __VA_ARGS__)

#define HOOK_MAKE(seId, aeId, seOffset, aeOffset, hookName, retName, ...)            \
	struct Hook_##hookName                                                           \
	{                                                                                \
		static retName __fastcall hooked(__VA_ARGS__);                               \
		static constexpr auto id = REL::RelocationID(seId, aeId);                    \
		static constexpr auto offset = REL::VariantOffset(seOffset, aeOffset, 0x00); \
		static inline REL::Relocation<decltype(hooked)> oldFunc;                     \
	};

#define HOOK_FUNC(managerName, hookName, ...) \
	__fastcall managerName::Hook_##hookName::hooked(__VA_ARGS__)
