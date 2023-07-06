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

#include "RateLimit.hpp"
#include "Helpers/Destruct.hpp"
#include "Exceptions.hpp"

/************************************************************************/

typedef SteamBot::RateLimiter RateLimiter;

/************************************************************************/
/*
 * Note: we explicity disallow destructing a RateLimiter, so we know
 * that the RateLimiter* stays valid, even without using shared/weak
 * pointers.
 */

static boost::fibers::mutex& mutex=*new boost::fibers::mutex;
static thread_local std::vector<RateLimiter*> cancelled;

/************************************************************************/

RateLimiter::RateLimiter(std::chrono::steady_clock::duration schedule_)
    : schedule(schedule_)
{
}

/************************************************************************/
/*
 * Checks the current cancel state for the calling thread.
 * If cancel=true, set cancel as well (returns previous state).
 */

bool RateLimiter::cancelAction(bool cancel)
{
    bool result=false;

    std::lock_guard<decltype(::mutex)> lock(::mutex);
    {
        for (const auto item : cancelled)
        {
            if (item==this)
            {
                result=true;
                break;
            }
        }
        if (cancel && !result)
        {
            cancelled.push_back(this);
        }
    }
    return result;
}

/************************************************************************/

void RateLimiter::cancel()
{
    if (!cancelAction(true))
    {
        condition.notify_all();
    }
}

/************************************************************************/

void RateLimiter::limit(std::function<void()> function)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    do
    {
        condition.wait_until(lock, nextAccess, [this]() {
            if (cancelAction(false))
            {
                throw SteamBot::OperationCancelledException();
            }
            return false;
        });
    }
    while (std::chrono::steady_clock::now()<nextAccess);

    {
        // We don't need to wake up the condition, since we're only
        // extending the wait. We can just as well let the existing
        // waits expire.

        SteamBot::ExecuteOnDestruct atEnd([this, &lock]() {
            nextAccess=std::chrono::steady_clock::now()+schedule;
        });
        function();
    }
}
