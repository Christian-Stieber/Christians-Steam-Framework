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

#include "Client/Module.hpp"
#include "WebAPI/ISteamDirectory/GetCMList.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_GetCMList : public SteamBot::Client::Module
    {
    public:
        TestModule_GetCMList() =default;
        virtual ~TestModule_GetCMList() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                auto cmlist=SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0);
                BOOST_LOG_TRIVIAL(debug) << "got GetCMList";
                boost::this_fiber::sleep_for(std::chrono::milliseconds(5000));
                cmlist=SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0);
                BOOST_LOG_TRIVIAL(debug) << "got GetCMList";
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_GetCMList::Init<TestModule_GetCMList> init;
}
