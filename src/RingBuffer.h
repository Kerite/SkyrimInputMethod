#pragma once

#include "Helpers/DebugHelper.h"
#include "Utils.h"

template <typename T, std::uint32_t N>
class RingBuffer
{
public:
	RingBuffer() :
		m_head(0), m_pool(nullptr) {}

	~RingBuffer()
	{
		if (m_pool != nullptr) {
			RE::free(m_pool);
		}
	}

	template <typename... Args>
	T* emplace_back(Args&&... args)
	{
		if (m_pool == nullptr) {
			DH_DEBUG("Before HeapAlloc");
			m_pool = RE::malloc(sizeof(T) * N);
			DH_DEBUG("After HeapAlloc");
		}
		T* result = nullptr;
		result = new (static_cast<T*>(m_pool) + m_head) T(std::forward<Args>(args)...);
		m_head += 1;
		if (m_head >= N)
			m_head = 0;
		return result;
	}

private:
	void* m_pool = nullptr;
	std::uint32_t m_head;
};