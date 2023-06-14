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

#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <memory>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace WebAPI
    {
        namespace ISteamDirectory
        {
            class GetCMList
            {
            public:
                std::chrono::system_clock::time_point when;

				std::vector<std::string> serverlist;

            public:
                GetCMList(boost::json::value&);		// private, use get() instead
                ~GetCMList();

            public:
                // Note: this MUST only used on the Asio thread, so I'm not using the Waiter interface here
                typedef std::function<void(std::shared_ptr<const GetCMList>)> CallbackType;
                static void get(unsigned int, CallbackType);

            public:
                // Temporary, until the new connection handling gets done
                static std::shared_ptr<const GetCMList> get(unsigned int);
            };
        }
    }
}


