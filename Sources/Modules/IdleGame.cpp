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
#include "Modules/Login.hpp"
#include "Modules/PlayGames.hpp"

/************************************************************************/
/*
 * This is just a temporary test module that tries to idle a hardcoded
 * game.
 */

/************************************************************************/

static const auto appId=static_cast<SteamBot::AppID>(383080);		// https://store.steampowered.com/app/383080/Sakura_Clicker/

/************************************************************************/

namespace
{
    class IdleGameModule : public SteamBot::Client::Module
    {
    public:
        IdleGameModule() =default;
        virtual ~IdleGameModule() =default;

        virtual void run() override;
    };

    IdleGameModule::Init<IdleGameModule> init;
}

/************************************************************************/

void IdleGameModule::run()
{
    getClient().launchFiber("IdleGameModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter.createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        while (true)
        {
            waiter.wait();
            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                SteamBot::Modules::PlayGames::Messageboard::PlayGame::play(appId);
            }
        }
    });
}
