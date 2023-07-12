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

#include <boost/fiber/properties.hpp>
#include <boost/fiber/algo/algorithm.hpp>
#include <boost/fiber/algo/round_robin.hpp>

/************************************************************************/
/*
 * This is a fiber-scheduler that counts the number of fibers...
 */

namespace SteamBot
{
    namespace ClientFiber
    {
        class Properties : public boost::fibers::fiber_properties
        {
        public:
            class Counter
            {
            private:
                boost::fibers::mutex mutex;
                boost::fibers::condition_variable condition;
                unsigned int counter;

            public:
                Counter()
                    : counter(0)
                {
                }

                void print() const
                {
                    BOOST_LOG_TRIVIAL(debug) << "Fiber count: " << counter;
                }

                void increase()
                {
                    counter++;
                    print();
                }

                void decrease()
                {
                    counter--;
                    print();
                    condition.notify_all();
                }

                void wait()
                {
                    BOOST_LOG_TRIVIAL(debug) << "Waiting for fibers...";
                    std::unique_lock<decltype(mutex)> lock(mutex);
                    condition.wait(lock, [this]() { return counter==1; } );		// ignore calling fiber, of course
                }
            };

            inline static thread_local Counter counter;

        private:
            std::thread::id myThread;

        public:
            Properties(boost::fibers::context* context)
                : fiber_properties(context),
                  myThread(std::this_thread::get_id())
            {
                counter.increase();
            }

            virtual ~Properties()
            {
                assert(myThread==std::this_thread::get_id());
                counter.decrease();
            }
        };

        class Scheduler : public boost::fibers::algo::algorithm_with_properties<Properties>
        {
        private:
            boost::fibers::algo::round_robin scheduler;

        public:
            virtual void awakened(boost::fibers::context* context, Properties&) noexcept
            {
                return scheduler.awakened(context);
            }

            virtual boost::fibers::context* pick_next() noexcept
            {
                return scheduler.pick_next();
            }

            virtual bool has_ready_fibers() const noexcept
            {
                return scheduler.has_ready_fibers();
            }

            virtual void suspend_until(std::chrono::steady_clock::time_point const& when) noexcept
            {
                return scheduler.suspend_until(when);
            }

            virtual void notify() noexcept
            {
                return scheduler.notify();
            }
        };
    }
}
