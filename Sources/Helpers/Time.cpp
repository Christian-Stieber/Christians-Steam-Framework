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

#include "Helpers/Time.hpp"

/************************************************************************/

std::string SteamBot::Time::toString(std::chrono::system_clock::time_point when, bool utc)
{
    const time_t timeBuffer=decltype(when)::clock::to_time_t(when);

    struct tm tmBuffer;
    if (utc)
    {
        gmtime_r(&timeBuffer, &tmBuffer);
    }
    else
    {
        localtime_r(&timeBuffer, &tmBuffer);
    }

    char buffer[200];
    auto size=strftime(buffer, sizeof(buffer), "%F %T %Z", &tmBuffer);

    return std::string(buffer, size);
}