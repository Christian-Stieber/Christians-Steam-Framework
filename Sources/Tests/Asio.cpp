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
#include "AsioYield.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

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
                boost::asio::deadline_timer timer(getClient().getIoContext());
                while (true)
                {
                    timer.expires_from_now(boost::posix_time::seconds(1));
                    timer.async_wait(boost::fibers::asio::yield);
                    BOOST_LOG_TRIVIAL(debug) << "asio timer fired";
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
