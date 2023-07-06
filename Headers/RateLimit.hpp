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

#include <chrono>

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

/************************************************************************/
/*
 * We give an EXTRA dose of rate-limiting to some activities, such
 * as inventory queries.
 *
 * This is thread-safe, so you can apply the limit across all clients
 * by letting them use the same instance.
 *
 * limit() will wait for a slot to become available, and call your
 * function. "lastAccess" will be updated when your function returns
 * (so waiting for something else, like HTTPClient actually running
 * your query will further delay things -- again, the assumption
 * is "slower is better").
 *
 * cancel() will cancel all active limit() calls on the same thread.
 *
 * Note: I expect instances to be "eternal", so destruction doesn't
 * work.
 */

/************************************************************************/

namespace SteamBot
{
    class RateLimiter
    {
    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        std::chrono::steady_clock::time_point nextAccess;

        const std::chrono::steady_clock::duration schedule;

    private:
        bool cancelAction(bool);

    public:
        RateLimiter(std::chrono::steady_clock::duration);
        ~RateLimiter() =delete;

    public:
        void cancel();
        void limit(std::function<void()>);
    };
}
