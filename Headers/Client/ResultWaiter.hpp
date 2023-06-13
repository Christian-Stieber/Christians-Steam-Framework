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
 * A "result waiter", for stuff that just runs async and returns
 * a single object when done.
 *
 * This is even threadsafe!
 *
 * Call gesResult() at any time; if the waiter hasn't awoken yet,
 * it will return nullptr, else a pointer to the result.
 *
 * As the producer, you can use setResult(), which will work
 * until you call completed().
 * Note that setResult() is not threadsafe, so know what
 * you're doing.
 */

namespace SteamBot
{
    class ResultWaiterBase : public Waiter::ItemBase
    {
    protected:
        std::atomic<bool> resultAvailable{false};

    private:
        virtual bool isWoken() const override
        {
            return resultAvailable;
        }

    protected:
        ResultWaiterBase(std::shared_ptr<SteamBot::Waiter>&& waiter)
            : ItemBase(std::move(waiter))
        {
        }

    public:
        void completed()
        {
            auto previous=resultAvailable.exchange(true);
            assert(!previous);
            wakeup();
        }

        virtual ~ResultWaiterBase() =default;
    };
}

/************************************************************************/

namespace SteamBot
{
    template <typename T> class ResultWaiter : public ResultWaiterBase
    {
    private:
        T result;

    public:
        ResultWaiter(std::shared_ptr<SteamBot::Waiter> waiter)
            : ResultWaiterBase(std::move(waiter))
        {
        }

        virtual ~ResultWaiter() =default;

    public:
        T& setResult()
        {
            assert(!resultAvailable.load());
            return result;
        }

    public:
        T* getResult()
        {
            return resultAvailable.load() ? &result : nullptr;
        }
    };
}
