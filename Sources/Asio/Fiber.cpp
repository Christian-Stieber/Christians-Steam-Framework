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

#include "Asio/Fiber.hpp"

#include "boost/examples/fiber/asio/round_robin.hpp"

/************************************************************************/

thread_local boost::fibers::asio::yield_t boost::fibers::asio::yield{};

/************************************************************************/

void boost::fibers::asio::setSchedulingAlgorithm(std::shared_ptr<boost::asio::io_context> ioContext)
{
    boost::fibers::use_scheduling_algorithm<boost::fibers::asio::round_robin>(ioContext);
}
