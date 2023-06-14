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

#include "Client/Whiteboard.hpp"
#include "Vector.hpp"

/************************************************************************/

SteamBot::Whiteboard::WaiterBase::WaiterBase(std::shared_ptr<SteamBot::WaiterBase> waiter_, SteamBot::Whiteboard& whiteboard_)
    : ItemBase(std::move(waiter_)), whiteboard(whiteboard_)
{
}

/************************************************************************/

SteamBot::Whiteboard::WaiterBase::~WaiterBase() =default;

/************************************************************************/

bool SteamBot::Whiteboard::WaiterBase::isWoken() const
{
    return hasChanged;
}

/************************************************************************/

void SteamBot::Whiteboard::WaiterBase::setChanged()
{
    if (!hasChanged)
    {
        hasChanged=true;
        wakeup();
    }
}

/************************************************************************/

SteamBot::Whiteboard::Whiteboard() =default;

/************************************************************************/

SteamBot::Whiteboard::~Whiteboard()
{
    assert(waiters.empty());	// must not destruct while there are still waiters!!
}

/************************************************************************/
/*
 * Performs "callback" on all waiters for type "key".
 *
 * This will also clean up released waiters.
 *
 * Pass a "nullptr" callback if you only want to perform the cleanup.
 */

void SteamBot::Whiteboard::action(const std::type_index& key, std::function<void(std::shared_ptr<WaiterBase>)> callback)
{
    const auto iterator=waiters.find(key);
    if (iterator!=waiters.end())
    {
        SteamBot::erase(iterator->second, [&callback](std::weak_ptr<WaiterBase>& waiter){
            auto locked=waiter.lock();
            if (locked)
            {
                if (callback) callback(locked);
                return false;
            }
            else
            {
                return true;
            }
        });
        if (iterator->second.empty())
        {
            waiters.erase(iterator);
        }
    }
}

/************************************************************************/

void SteamBot::Whiteboard::didChange(const std::type_index& key)
{
    action(key, [](std::shared_ptr<WaiterBase> item){
        item->setChanged();
    });
}
