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

/************************************************************************/
/*
 * Note: the existing Connection/ stuff is fiber based, and I'll
 * leave it at that... for now at least.
 */

/************************************************************************/

#include "Asio/Asio.hpp"
#include "Asio/Fiber.hpp"

/************************************************************************/

typedef SteamBot::Asio Asio;

/************************************************************************/

Asio::~Asio()
{
    assert(false);
}

/************************************************************************/

Asio::Asio()
    : ioContext(std::make_shared<boost::asio::io_context>())
{
    thread=std::thread([this](){
        BOOST_LOG_TRIVIAL(debug) << "Asio: running thread";
        boost::fibers::asio::setSchedulingAlgorithm(ioContext);
        auto work=boost::asio::make_work_guard(ioContext);
        ioContext->run();
        BOOST_LOG_TRIVIAL(debug) << "Asio: exiting thread";
    });
}

/************************************************************************/

Asio& Asio::get()
{
    static Asio& instance=*new Asio();
    return instance;
}

/************************************************************************/
/*
 * Check whether we are on the asio thread
 */

bool Asio::isThread()
{
    return get().thread.get_id()==std::this_thread::get_id();
}

/************************************************************************/

boost::asio::io_context& Asio::getIoContext()
{
    return *(get().ioContext);
}
