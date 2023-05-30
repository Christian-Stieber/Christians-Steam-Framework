/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

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
