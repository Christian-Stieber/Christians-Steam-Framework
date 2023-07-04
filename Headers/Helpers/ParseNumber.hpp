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

#include <string_view>
#include <charconv>

/************************************************************************/

namespace SteamBot
{
    template <typename T> bool parseNumber(std::string_view string, T& number)
        requires ((std::is_integral_v<T> && !std::is_same_v<T, bool>) || (std::is_floating_point_v<T>))
    {
        const auto last=string.data()+string.size();
        const auto result=std::from_chars(string.data(), last, number);
        return result.ec==std::errc() && result.ptr==last;
    }

    template <typename T> bool parseNumberPrefix(std::string_view& string, T& number)
        requires ((std::is_integral_v<T> && !std::is_same_v<T, bool>) || (std::is_floating_point_v<T>))
    {
        const auto first=string.data();
        const auto result=std::from_chars(first, first+string.size(), number);
        if (result.ec==std::errc())
        {
            string.remove_prefix(result.ptr-first);
            return true;
        }
        return false;
    }
}
