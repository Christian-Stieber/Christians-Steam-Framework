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

#include "UI/UI.hpp"
#include "AsioThread.hpp"
#include "Client/Module.hpp"
#include "Config.hpp"

#include <iostream>

/************************************************************************/

namespace
{
    class UIThread : public SteamBot::AsioThread
    {
    public:
        UIThread()
        {
            launch();
        }

        virtual ~UIThread() =default;

    public:
        static UIThread& get()
        {
            static UIThread* const thread=new UIThread();
            return *thread;
        }
    };
}

/************************************************************************/

SteamBot::UI::UIWaiterBase::UIWaiterBase(SteamBot::Waiter& waiter_)
    : ItemBase(waiter_)
{
}

/************************************************************************/

SteamBot::UI::UIWaiterBase::~UIWaiterBase() =default;

/************************************************************************/

void SteamBot::UI::UIWaiterBase::install(std::shared_ptr<SteamBot::Waiter::ItemBase> item)
{
    assert(item.get()==this);
    self=std::dynamic_pointer_cast<UIWaiterBase>(item);
    this->ItemBase::install(item);
}

/************************************************************************/

bool SteamBot::UI::GetSteamguardCode::isWoken() const
{
    if (params)
    {
        std::lock_guard<decltype(params->mutex)> lock(params->mutex);
        return params->code.length()==5;
    }
    return false;
}

/************************************************************************/

std::string SteamBot::UI::GetSteamguardCode::fetch()
{
    std::string result;
    if (params)
    {
        std::lock_guard<decltype(params->mutex)> lock(params->mutex);
        result.swap(params->code);
    }
    return result;
}

/************************************************************************/
/*
 * ToDo: we really need to make this more async, so we can abort the
 * operation...
 */

void SteamBot::UI::GetSteamguardCode::execute()
{
    UIThread::get().ioContext.post([params=params, self=self.lock()](){
        {
            std::string entered;
            while (!params->abort && entered.length()!=5)
            {
                std::cout << "Enter SteamGuard code for account " << params->user << ": ";
                std::cin >> entered;
            }
            std::lock_guard<decltype(params->mutex)> lock(params->mutex);
            params->code=std::move(entered);
        }
        self->wakeup();
    });
}

/************************************************************************/

SteamBot::UI::GetSteamguardCode::GetSteamguardCode(SteamBot::Waiter& waiter_)
    : UIWaiterBase(waiter_)
{
    assert(!params);
    params=std::make_shared<Params>();
    params->user=SteamBot::Config::SteamAccount::get().user;
}

/************************************************************************/

SteamBot::UI::GetSteamguardCode::~GetSteamguardCode()
{
    if (params)
    {
        params->abort=true;
    }
}

/************************************************************************/

std::shared_ptr<SteamBot::UI::GetSteamguardCode> SteamBot::UI::GetSteamguardCode::create(SteamBot::Waiter& waiter)
{
    auto item=waiter.createWaiter<GetSteamguardCode>();
    item->execute();
    return item;
}
