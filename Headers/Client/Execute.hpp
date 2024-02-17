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

#include "Client/Waiter.hpp"
#include "Client/Condition.hpp"

#include <queue>
#include <functional>

/************************************************************************/
/*
 * Execute a function on the fiber, and wait for completion
 */

namespace SteamBot
{
    class Execute : public Waiter::ItemBase
    {
    public:
        typedef std::shared_ptr<Execute> WaiterType;

    public:
        class FunctionBase
        {
        public:
            typedef std::shared_ptr<FunctionBase> Ptr;

        private:
            SteamBot::ConditionVariable condition;
            bool completed=false;

        protected:
            FunctionBase();

        public:
            virtual ~FunctionBase();

            virtual void execute(Ptr) =0;

            void wait();
            void complete();
        };

    private:
        mutable boost::fibers::mutex mutex;
        std::queue<FunctionBase::Ptr> queue;

    public:
        virtual bool isWoken() const override;

    public:
        Execute(std::shared_ptr<SteamBot::WaiterBase>&& waiter_)
            : ItemBase(std::move(waiter_))
        {
        }

        virtual ~Execute() =default;

    public:
        // Call this on the sending fiber
        void enqueue(FunctionBase::Ptr);

    public:
        // API on the receiving fiber
        void run();

    public:
        static WaiterType createWaiter(SteamBot::Waiter& waiter)
        {
            return waiter.createWaiter<Execute>();
        }
    };
}
