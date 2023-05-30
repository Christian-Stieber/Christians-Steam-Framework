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

#include "AsioThread.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::AsioThread AsioThread;

/************************************************************************/

AsioThread::AsioThread() =default;

/************************************************************************/

void AsioThread::launch()
{
    thread=std::thread([this]{
        BOOST_LOG_TRIVIAL(debug) << "AsioThread " << boost::typeindex::type_id_runtime(*this).pretty_name() << " launched";
        auto guard=boost::asio::make_work_guard(ioContext);
        ioContext.run();
        BOOST_LOG_TRIVIAL(debug) << "AsioThread " << boost::typeindex::type_id_runtime(*this).pretty_name() << " terminating";
    });
}

/************************************************************************/

AsioThread::~AsioThread()
{
    ioContext.stop();
    if (thread.joinable())
    {
        thread.join();
    }
}
