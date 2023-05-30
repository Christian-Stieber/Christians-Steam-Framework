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
#include "Client/Messageboard.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_Messageboard : public SteamBot::Client::Module
    {
    public:
        class TickMessage { };

    public:
    TestModule_Messageboard() =default;
        virtual ~TestModule_Messageboard() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                for (int i=0; i<5; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::seconds(1));
                    BOOST_LOG_TRIVIAL(debug) << "messageboard: sending TickMessage";
                    getClient().messageboard.send(std::make_shared<TickMessage>());
                }
            });
            getClient().launchFiber([this](){
                SteamBot::Waiter waiter;
                auto ticker=waiter.createWaiter<SteamBot::Messageboard::Waiter<TickMessage>>(getClient().messageboard);
                while (true)
                {
                    waiter.wait();
                    if (ticker->fetch())
                    {
                        BOOST_LOG_TRIVIAL(debug) << "messageboard: received TickMessage";
                    }
                }
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_Messageboard::Init<TestModule_Messageboard> init;
}
