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

#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * This is the thread that handles all asio-related stuff
 */

namespace SteamBot
{
    class Asio
    {
    private:
        std::thread thread;
        std::shared_ptr<boost::asio::io_context> ioContext;

    private:
        Asio();
        ~Asio();

        static Asio& get();

    public:
        static bool isThread();

        static boost::asio::io_context& getIoContext();

    public:
        template <typename HANDLER> static void post(std::string_view name, HANDLER handler)
        {
            bool noLogging=(name=="Connections::writePacket");
            if (!noLogging) BOOST_LOG_TRIVIAL(debug) << "Asio: posting \"" << name << "\"";
            getIoContext().post([name, noLogging, handler=std::move(handler)]() mutable {
                if (!noLogging) BOOST_LOG_TRIVIAL(debug) << "Asio: thread is running \"" << name << "\"";
                handler();
                if (!noLogging) BOOST_LOG_TRIVIAL(debug) << "Asio: thread is exiting \"" << name << "\"";
            });
        }
    };
}
