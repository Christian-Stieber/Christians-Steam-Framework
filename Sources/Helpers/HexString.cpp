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

#include "Helpers/HexString.hpp"

#include <cassert>

/************************************************************************/

static char hexDigit(uint8_t digit)
{
	if (digit<10) return '0'+digit;
	if (digit<16) return 'a'-10+digit;
    assert(false);
	return '?';
}

/************************************************************************/

std::string SteamBot::makeHexString(std::span<const std::byte> bytes, char separator)
{
    std::string result;
    {
        size_t size=2*bytes.size();
        if (separator!='\0' && bytes.size()>0)
        {
            size+=bytes.size()-1;
        }
        result.reserve(size);
    }
    for (const auto byte: bytes)
    {
        if (separator!='\0' && result.empty()) result+=separator;
        result+=hexDigit(static_cast<uint8_t>(byte)>>4);
        result+=hexDigit(static_cast<uint8_t>(byte)&0x0f);
    }
    return result;
}
