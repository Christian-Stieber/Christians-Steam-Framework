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

/************************************************************************/

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

/************************************************************************/
/*
 * This is a "counter" that counts objects that are created from
 * it, until they are destroyed.
 */

namespace SteamBot
{
    class Counter
    {
    private:
        boost::fibers::condition_variable condition;
        boost::fibers::mutex mutex;
        unsigned int counter=0;

    public:
        class Handle
        {
        private:
            Counter& counter;

        public:
            Handle(Counter& counter_)
                : counter(counter_)
            {
                std::lock_guard<boost::fibers::mutex> lock(counter.mutex);
                counter.counter++;
            }

            Handle(const Handle& other)
                : counter(other.counter)
            {
                std::lock_guard<boost::fibers::mutex> lock(counter.mutex);
                counter.counter++;
            }

            ~Handle()
            {
                std::lock_guard<boost::fibers::mutex> lock(counter.mutex);
                if (--counter.counter==0)
                {
                    counter.onEmpty();
                    counter.condition.notify_all();
                }
            }
        };

        Handle operator()()
        {
            return Handle(*this);
        }

    public:
        void wait()
        {
            std::unique_lock<boost::fibers::mutex> lock(mutex);
            condition.wait(lock, [this]() { return counter==0; });
        }

        unsigned int getCount()
        {
            std::unique_lock<boost::fibers::mutex> lock(mutex);
            return counter;
        }

    public:
        Counter() =default;
        virtual ~Counter() =default;

    private:
        virtual void onEmpty() { }
    };
}
