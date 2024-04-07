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

#include <sstream>

#include <cassert>
#include <cmath>

/************************************************************************/

struct tm& SteamBot::Time::toCalendar(std::chrono::system_clock::time_point when, bool utc, struct tm& myTm)
{
    const time_t timestamp=decltype(when)::clock::to_time_t(when);
    if (utc)
    {
#ifdef __linux__
		auto result=gmtime_r(&timestamp, &myTm);
		assert(result!=nullptr);
#else
		auto result=_gmtime64_s(&myTm, &timestamp);
		assert(result==0);
#endif
    }
    else
    {
#ifdef __linux__
		auto result=localtime_r(&timestamp, &myTm);
		assert(result!=nullptr);
#else
		auto result=_localtime64_s(&myTm, &timestamp);
		assert(result==0);
#endif
    }

    return myTm;
}

/************************************************************************/

std::string SteamBot::Time::toString(std::chrono::system_clock::time_point when, bool utc)
{
    struct tm myTm;
    char buffer[200];
    auto size=strftime(buffer, sizeof(buffer), "%F %T", &toCalendar(when, utc, myTm));

    return std::string(buffer, size);
}

/************************************************************************/

std::string SteamBot::Time::toString(std::chrono::minutes minutes)
{
    std::ostringstream string;
    if (minutes.count()<60)
    {
        string << minutes.count() << " minutes";
    }
    else if (minutes.count()<10*60)
    {
        string << round(10.0*minutes.count()/60.0)/10.0 << " hours";
    }
    else
    {
        string << round(minutes.count()/60.0) << " hours";
    }
    return std::move(string).str();
}
