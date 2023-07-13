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
 *
 * Note: https://github.com/boostorg/fiber/issues/308#
 */

/************************************************************************/

namespace SteamBot
{
    namespace ClientFiber
    {
        class Tracker
        {
        public:
            boost::fibers::condition_variable condition;
            unsigned int counter=0;
            unsigned int baseCounter=0;

        public:
            void wait()
            {
                boost::fibers::mutex mutex;
                std::unique_lock<decltype(mutex)> lock(mutex);
                condition.wait(lock, [this](){ return counter<=baseCounter; });
            }

            void print()
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
                condition.notify_one();
            }

        public:
            void setBaseCounter()
            {
                boost::this_fiber::yield();
                baseCounter=counter;
            }
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace ClientFiber
    {
        class Properties : public boost::fibers::fiber_properties
        {
        private:
            std::shared_ptr<Tracker> tracker;

        public:
            Properties(boost::fibers::context* context)
                : fiber_properties(context)
            {
                assert(false);
            }

            Properties(boost::fibers::context* context, std::shared_ptr<Tracker> tracker_)
                : fiber_properties(context), tracker(std::move(tracker_))
            {
                tracker->increase();
            }

            ~Properties()
            {
                tracker->decrease();
            }
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace ClientFiber
    {
        class Scheduler : public boost::fibers::algo::algorithm_with_properties<Properties>
        {
        public:
            boost::fibers::algo::round_robin scheduler;
            std::shared_ptr<Tracker> tracker{std::make_shared<Tracker>()};

        public:
            Scheduler(Scheduler*& self)
            {
                self=this;
            }

        public:
            virtual void awakened(boost::fibers::context* context, Properties&) noexcept // override
            {
                return scheduler.awakened(context);
            }

            virtual boost::fibers::context* pick_next() noexcept override
            {
                return scheduler.pick_next();
            }

            virtual bool has_ready_fibers() const noexcept override
            {
                return scheduler.has_ready_fibers();
            }

            virtual void suspend_until(std::chrono::steady_clock::time_point const& when) noexcept override
            {
                return scheduler.suspend_until(when);
            }

            virtual void notify() noexcept override
            {
                return scheduler.notify();
            }

        public:
            virtual boost::fibers::fiber_properties* new_properties(boost::fibers::context* context) override
            {
                return new Properties(context, tracker);
            }

            void wait()
            {
                tracker->wait();
            }

            void setBaseCounter()
            {
                tracker->setBaseCounter();
            }
        };
    }
}
