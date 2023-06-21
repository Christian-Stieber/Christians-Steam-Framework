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

#include "Client/Client.hpp"
#include "Client/Sleep.hpp"
#include "Exceptions.hpp"

/************************************************************************/

namespace
{
    class Sleeper
    {
    private:
        boost::fibers::condition_variable condition;
        boost::fibers::mutex mutex;
        bool cancelled=false;

    public:
        void operator()(std::chrono::milliseconds duration)
        {
            std::unique_lock<decltype(mutex)> lock(mutex);
            condition.wait_for(lock, duration, [this]() { return cancelled; });
            if (cancelled)
            {
                throw SteamBot::OperationCancelledException();
            }
        }

        void cancel()
        {
            cancelled=true;
            condition.notify_one();
        }
    };
}

/************************************************************************/

void SteamBot::sleep(std::chrono::milliseconds duration)
{
    Sleeper sleeper;
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(sleeper);
    sleeper(duration);
}
