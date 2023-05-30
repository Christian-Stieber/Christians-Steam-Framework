#pragma once

#include <cstddef>
#include <cstdint>

#include <span>
#include <array>
#include <vector>

/************************************************************************/
/*
 * Get a "usable" data pointer, from std::byte stuff
 */

namespace Steam
{
	template <typename T=uint8_t> inline const T* getUsableBytes(const std::byte* ptr)
	{
		return static_cast<const T*>(static_cast<const void*>(ptr));
	}

	template <typename T=uint8_t> inline T* getUsableBytes(std::byte* ptr)
	{
		return static_cast<T*>(static_cast<void*>(ptr));
	}

	template <typename T=uint8_t, typename U> inline auto getUsableBytes(U& data)
	{
		return getUsableBytes<T>(data.data());
	}
}
