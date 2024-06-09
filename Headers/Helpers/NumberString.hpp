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
#include <concepts>

/************************************************************************/

namespace SteamBot
{
    template <std::integral T> std::string toString_(T);

    template <std::integral T> std::string toString(T value)
    {
        if constexpr (std::is_signed_v<T>)
        {
            return toString_<long long>(value);
        }
        else
        {
            return toString_<unsigned long long>(value);
        }
    }
}

/************************************************************************/
/*
 * This is mostly for UI purposes; it turns a byte-size into a nice
 * string.
 */

namespace SteamBot
{
    std::string printSize(size_t);
}
