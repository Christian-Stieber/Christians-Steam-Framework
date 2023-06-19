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
#include <compare>

/************************************************************************/

namespace SteamBot
{
    std::weak_ordering caseInsensitiveStringCompare(std::string_view, std::string_view);

    inline bool caseInsensitiveStringCompare_less(std::string_view left, std::string_view right)
    {
        return caseInsensitiveStringCompare(left, right)==std::weak_ordering::less;
    }

    inline bool caseInsensitiveStringCompare_equal(std::string_view left, std::string_view right)
    {
        return caseInsensitiveStringCompare(left, right)==std::weak_ordering::equivalent;
    }
}
