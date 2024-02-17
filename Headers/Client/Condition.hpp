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

#include "Exceptions.hpp"

#include <atomic>

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

/************************************************************************/
/*
 * This is NOT a waiter item, just a cancelable condition variable
 */

namespace SteamBot
{
    class ConditionVariable
    {
    public:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;

    private:
        std::atomic<bool> cancelled{false};

    public:
        void cancel()
        {
            if (!cancelled.exchange(true))
            {
                condition.notify_all();
            }
        }

    public:
        ConditionVariable() =default;
        ~ConditionVariable() =default;

    public:
        template <class PRED> void wait(std::unique_lock<decltype(mutex)>& lock, PRED predicate)
        {
            while (true)
            {
                if (cancelled)
                {
                    throw OperationCancelledException();
                }
                if (predicate())
                {
                    return;
                }
                condition.wait(lock);
            }
        }

    public:
        template <class PRED> std::unique_lock<decltype(mutex)> wait(PRED&& predicate)
        {
            std::unique_lock<decltype(mutex)> lock(mutex);
            wait(lock, std::forward<PRED>(predicate));
            return lock;
        }

        void notify_one()
        {
            condition.notify_one();
        }

        void notify_all()
        {
            condition.notify_all();
        }
    };
}
