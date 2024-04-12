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

#include "Helpers/NumberString.hpp"

#include <charconv>
#include <cassert>

/************************************************************************/

template <std::integral T> static std::string SteamBot::toString_(T value)
{
    char buffer[64];
    auto result=std::to_chars(buffer, buffer+sizeof(buffer), value);
    assert(result.ec==std::errc());
    return std::string(buffer, result.ptr);
}

template static std::string SteamBot::toString_(long long);
template static std::string SteamBot::toString_(unsigned long long);
