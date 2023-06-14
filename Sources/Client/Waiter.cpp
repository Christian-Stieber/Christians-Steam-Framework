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

#include "Client/Waiter.hpp"
#include "Vector.hpp"
#include "Exceptions.hpp"

/************************************************************************/

typedef SteamBot::WaiterBase WaiterBase;
typedef WaiterBase::ItemBase ItemBase;

typedef SteamBot::Waiter Waiter;

/************************************************************************/

WaiterBase::WaiterBase() =default;
WaiterBase::~WaiterBase() =default;

Waiter::Waiter() =default;
Waiter::~Waiter() =default;

/************************************************************************/

ItemBase::ItemBase(std::shared_ptr<WaiterBase> waiter_)
    : waiter(std::move(waiter_))
{
}

ItemBase::~ItemBase() =default;

/************************************************************************/

void ItemBase::wakeup()
{
    if (auto locked=waiter.lock())
    {
        locked->wakeup();
    }
}

/************************************************************************/

void ItemBase::install(std::shared_ptr<ItemBase>)
{
}

/************************************************************************/

void WaiterBase::cancel()
{
    if (!cancelled)
    {
        cancelled=true;
        wakeup();
    }
}

/************************************************************************/

bool WaiterBase::isWoken()
{
    bool woken=false;
    SteamBot::erase(items, [&woken](const std::weak_ptr<ItemBase>& item){
        auto locked=item.lock();
        if (locked)
        {
            woken|=locked->isWoken();
            return false;
        }
        else
        {
            return true;
        }
    });
    return woken;
}

/************************************************************************/

void Waiter::wait()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this](){ return cancelled || isWoken(); });
    if (cancelled)
    {
        throw OperationCancelledException();
    }
}

/************************************************************************/
/*
 * Returne false if the timeout was reached
 */

bool Waiter::wait(std::chrono::milliseconds timeout)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    bool result=condition.wait_for(lock, timeout, [this](){ return cancelled || isWoken(); });
    if (cancelled)
    {
        throw OperationCancelledException();
    }
    return result;
}

/************************************************************************/

void Waiter::wakeup()
{
    condition.notify_all();
}

/************************************************************************/

std::shared_ptr<SteamBot::Waiter> Waiter::create()
{
    return std::shared_ptr<SteamBot::Waiter>(new Waiter);
}
