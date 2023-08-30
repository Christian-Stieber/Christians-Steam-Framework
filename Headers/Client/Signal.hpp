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

#include <atomic>

/************************************************************************/
/*
 * This is a simple "signal" waiter
 */

namespace SteamBot
{
    class Signal : public Waiter::ItemBase
    {
    public:
        typedef std::shared_ptr<Signal> WaiterType;

    private:
        std::atomic<bool> signaled{false};

    public:
        virtual bool isWoken() const override
        {
            return signaled.load();
        }

    public:
        Signal(std::shared_ptr<SteamBot::WaiterBase>&& waiter_)
            : ItemBase(std::move(waiter_))
        {
        }

        virtual ~Signal() =default;

    public:
        void signal()
        {
            if (!signaled.exchange(true))
            {
                wakeup();
            }
        }

        bool testAndClear()
        {
            return signaled.exchange(false);
        }

    public:
        static WaiterType createWaiter(SteamBot::Waiter& waiter)
        {
            return waiter.createWaiter<Signal>();
        }
    };
}
