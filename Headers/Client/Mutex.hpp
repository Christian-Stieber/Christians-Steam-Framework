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

#include "Client/Condition.hpp"

/************************************************************************/
/*
 * This is NOT a waiter item, just a cancelable mutex
 */

namespace SteamBot
{
    class Mutex
    {
    private:
        SteamBot::ConditionVariable condition;
        bool locked=false;

    public:
        void cancel()
        {
            condition.cancel();
        }

        void lock()
        {
            auto myLock=condition.wait([this](){ return !locked; });
            assert(!locked);
            locked=true;
        }

        void unlock()
        {
            assert(locked);
            {
                std::lock_guard<decltype(condition.mutex)> myLock(condition.mutex);
                locked=false;
            }
            condition.notify_all();
        }
    };
}
