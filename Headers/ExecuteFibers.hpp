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

#include <functional>

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

namespace SteamBot
{
    class ExecuteFibers
    {
    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        unsigned int counter=0;

    public:
        ExecuteFibers();
        ~ExecuteFibers();

    public:
        void run(std::function<void()>);
    };
}
