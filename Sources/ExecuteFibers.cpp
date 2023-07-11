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

#include "ExecuteFibers.hpp"

#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

SteamBot::ExecuteFibers::ExecuteFibers() =default;

/************************************************************************/

void SteamBot::ExecuteFibers::run(std::function<void()> function)
{
    counter++;
    boost::fibers::fiber([this, function=std::move(function)] {
        try
        {
            function();
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(error) << "ExecuteFibers: fiber exception " << boost::current_exception_diagnostic_information();
        }
        counter--;
        condition.notify_one();
    }).detach();
}

/************************************************************************/

SteamBot::ExecuteFibers::~ExecuteFibers()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this]() { return counter==0; });
    BOOST_LOG_TRIVIAL(debug) << "ExecuteFibers: completed";
};
