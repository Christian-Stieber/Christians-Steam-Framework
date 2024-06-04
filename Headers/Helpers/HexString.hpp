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

#include <string>
#include <span>
#include <cstddef>
#include <cstdint>

/************************************************************************/

namespace SteamBot
{
    std::string makeHexString(std::span<const std::byte>, char separator='\0');

    inline std::string makeHexString(std::span<const std::uint8_t> bytes, char separator='\0')
    {
        return makeHexString(std::span<const std::byte>{static_cast<const std::byte*>(static_cast<const void*>(bytes.data())), bytes.size()}, separator);
    }
}
