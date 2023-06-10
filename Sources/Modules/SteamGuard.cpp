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

#include "Client/Module.hpp"
#include "Modules/SteamGuard.hpp"
#include "UI/UI.hpp"
#include "Config.hpp"

#include <mutex>
#include <unordered_set>

/************************************************************************/

namespace
{
    std::mutex mutex;
    std::unordered_set<std::string> needsSteamGuard;		// account names
}

/************************************************************************/

namespace
{
    class SteamGuardModule : public SteamBot::Client::Module
    {
    private:
        bool isFlagged();

    public:
        SteamGuardModule() =default;
        virtual ~SteamGuardModule() =default;

        virtual void run() override;
    };

    SteamGuardModule::Init<SteamGuardModule> init;
}

/************************************************************************/
/*
 * Check whether the current account needs SteamGuard, and also reset
 * the registration.
 */

bool SteamGuardModule::isFlagged()
{
    const auto& account=SteamBot::Config::SteamAccount::get().user;
    std::lock_guard<decltype(mutex)> lock(mutex);
    return (needsSteamGuard.erase(account)!=0);
}

/************************************************************************/

void SteamGuardModule::run()
{
    getClient().launchFiber("SteamGuardModule::run", [this](){
        std::string code;
        if (isFlagged())
        {
            auto waiter=SteamBot::Waiter::create();
            auto cancellation=getClient().cancel.registerObject(*waiter);

            auto steamGuardCode=SteamBot::UI::Thread::requestPassword(waiter, SteamBot::UI::Base::PasswordType::SteamGuard_EMail);

            waiter->wait();
            code=std::move(*(steamGuardCode->getResult()));
        }
        typedef SteamBot::Modules::SteamGuard::Whiteboard::SteamGuardCode SteamGuardCode;
        getClient().whiteboard.set<SteamGuardCode>(std::move(code));
    });
}

/************************************************************************/

void SteamBot::Modules::SteamGuard::registerAccount()
{
    const auto& account=SteamBot::Config::SteamAccount::get().user;
    BOOST_LOG_TRIVIAL(info) << "SteamGuard: registering account " << account;
    std::lock_guard<decltype(mutex)> lock(mutex);
    needsSteamGuard.insert(account);
}
