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

#include <cassert>

/************************************************************************/
/*
 * This is an attempt to get the start of the next Steam day.
 *
 * As I understand it, Steam does not observe DST.
 *
 * This will update the time_point that you're passing in, instead
 * using now(), to help avoid races.
 *
 * Also, be aware that our local clock and Steams are likely not in
 * sync, so you might want to add a bit of leeway.
 */

void SteamBot::Time::nextSteamDay(std::chrono::system_clock::time_point& timestamp)
{
    static const int UTCOffset=7;

    assert(timestamp.time_since_epoch().count()!=0);

    const time_t now=std::chrono::system_clock::to_time_t(timestamp);
    struct tm tmBuffer;

    {
#ifdef __linux__
        auto result=gmtime_r(&now, &tmBuffer);
        assert(result!=nullptr);
#else
        auto result=_gmtime64_s(&tmBuffer, &now);
        assert(result==0);
#endif
    }

    tmBuffer.tm_sec=0;
    tmBuffer.tm_min=0;
    tmBuffer.tm_isdst=0;

    if (tmBuffer.tm_hour>=10+UTCOffset)
    {
        tmBuffer.tm_mday+=1;
    }
    tmBuffer.tm_hour=10+UTCOffset;

    std::time_t then;
#ifdef __linux__
    then=timegm(&tmBuffer);
#else
    then=_mkgmtime(&tmBuffer);
#endif

    timestamp=std::chrono::system_clock::from_time_t(then);
}
