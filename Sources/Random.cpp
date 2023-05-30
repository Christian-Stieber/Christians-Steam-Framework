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

#include "Random.hpp"

#include <boost/fiber/mutex.hpp>

/************************************************************************/

void SteamBot::Random::makeRandomNumber(std::function<void(SteamBot::Random::Generator&)> callback)
{
    static boost::fibers::mutex mutex;
    static Generator generator((std::random_device())());

    std::lock_guard<decltype(mutex)> lock(mutex);
    callback(generator);
}

/************************************************************************/

SteamBot::Random::Generator::result_type SteamBot::Random::generateRandomNumber()
{
    Generator::result_type result;
    makeRandomNumber([&result](Generator& generator) {
        result=generator();
    });
    return result;
}
