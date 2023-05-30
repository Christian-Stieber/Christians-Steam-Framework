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
#include "HTTPClient.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_HTTPClient : public SteamBot::Client::Module
    {
    public:
        TestModule_HTTPClient() =default;
        virtual ~TestModule_HTTPClient() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                // ToDo: why does this not fail ssl verification?
                auto response=SteamBot::HTTPClient::query("https://gatekeeper:2443/public/Test").get();
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_HTTPClient::Init<TestModule_HTTPClient> init;
}
