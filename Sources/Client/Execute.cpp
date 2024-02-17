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

#include "Client/Execute.hpp"
#include "Client/Client.hpp"

/************************************************************************/

bool SteamBot::Execute::isWoken() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return !queue.empty();
}

/************************************************************************/

SteamBot::Execute::FunctionBase::FunctionBase() =default;
SteamBot::Execute::FunctionBase::~FunctionBase() =default;

/************************************************************************/

void SteamBot::Execute::FunctionBase::wait()
{
    // ToDo: I suppose we should improve the cancellation so we can add objects
    // after a quit as been issued, but for now we#ll have to do it like this

    auto& client=SteamBot::Client::getClient();
    auto cancellation=client.cancel.registerObject(condition);
    if (client.isQuitting())
    {
        condition.cancel();
    }
    condition.wait([this](){ return completed; });
}

/************************************************************************/

void SteamBot::Execute::FunctionBase::complete()
{
    {
        std::lock_guard<decltype(condition.mutex)> lock(condition.mutex);
        assert(!completed);
        completed=true;
    }
    condition.notify_one();
}

/************************************************************************/

void SteamBot::Execute::enqueue(FunctionBase::Ptr function)

{
    std::lock_guard<decltype(mutex)> lock(mutex);
    queue.emplace(function);
    wakeup();
}

/************************************************************************/

void SteamBot::Execute::run()
{
    FunctionBase::Ptr function;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (!queue.empty())
        {
            function=std::move(queue.front());
            queue.pop();
        }
    }
    if (function)
    {
        auto function_=function.get();
        function_->execute(std::move(function));
    }
}
