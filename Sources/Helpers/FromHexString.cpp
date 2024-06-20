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

/************************************************************************/

static uint16_t digit(char c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='a' && c<='f') return c-'a'+10;
    if (c>='A' && c<='F') return c-'A'+10;
    return 0x100;
}

/************************************************************************/

bool SteamBot::fromHexString(std::string_view string, std::span<std::byte> result)
{
    if (string.size()!=2*result.size()) return false;
    for (size_t i=0; i<result.size(); i++)
    {
        uint16_t byte=(digit(string[2*i+0]) << 4) | digit(string[2*i+1]);
        if ((byte & ~0xff)!=0) return false;
        result[i]=static_cast<std::byte>(byte & 0xff);
    }
    return true;
}
