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
#include "Client/Whiteboard.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule : public SteamBot::Client::Module
    {
    public:
        TestModule() =default;
        virtual ~TestModule() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                for (int i=0; i<5; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::seconds(1));
                    getClient().whiteboard.set<int>(100+i);
                }
            });
            getClient().launchFiber([this](){
                for (unsigned i=0; i<7; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::milliseconds(700));
                    getClient().whiteboard.set<unsigned>(200+i);
                }
            });
            getClient().launchFiber([this](){
                SteamBot::Waiter waiter;
                auto Int=waiter.createWaiter<SteamBot::Whiteboard::Waiter<int>>(getClient().whiteboard);
                auto Unsigned=waiter.createWaiter<SteamBot::Whiteboard::Waiter<unsigned>>(getClient().whiteboard);
                while (true)
                {
                    BOOST_LOG_TRIVIAL(debug) << "whiteboard: " << Int->get(0) << ", " << Unsigned->get(0);
                    waiter.wait();
                }
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule::Init<TestModule> init;
}
