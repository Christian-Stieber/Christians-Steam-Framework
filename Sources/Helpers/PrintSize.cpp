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

#include <cassert>

/************************************************************************/
/*
 * For now, we output a "xxx bytes", "xxx KiB" or "xxx MiB".
 *
 * KiB/MiB will get one decimal digit, which I'm trying to determine
 * with integer arithmetic.
 */

std::string SteamBot::printSize(size_t size)
{
    const char* unit=nullptr;

    if (size>=1024*1024)
    {
        size=((size+50*1000)*10)/(1024*1024);
        unit="MiB";
    }
    else if (size>=1024)
    {
        size=((size+50)*10)/(1024);
        unit="KiB";
    }

    auto result=SteamBot::toString(size);

    if (unit!=nullptr)
    {
        // do the "floating point" correction
        assert(!result.empty());
        const char decimal=result.back();
        result.pop_back();
        if (decimal!='0')
        {
            result+='.';
            result+=decimal;
        }
    }
    else
    {
        unit="bytes";
    }

    result+=' ';
    result+=unit;

    return result;
}
