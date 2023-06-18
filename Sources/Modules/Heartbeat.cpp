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

#include "Modules/Connection.hpp"
#include "Client/Module.hpp"
#include "Modules/Login.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

/************************************************************************/

namespace
{
    class HeartbeatModule : public SteamBot::Client::Module
    {
    public:
        HeartbeatModule() =default;
        virtual ~HeartbeatModule() =default;

        virtual void run() override;
    };

    HeartbeatModule::Init<HeartbeatModule> init;
}

/************************************************************************/

void HeartbeatModule::run()
{
    getClient().launchFiber("HeartbeatModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;

        typedef SteamBot::Modules::Connection::Whiteboard::LastMessageSent LastMessageSent;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LastMessageSent>> lastMessageSent;

        {
            auto& whiteboard=SteamBot::Client::getClient().whiteboard;
            loginStatus=waiter->createWaiter<decltype(loginStatus)::element_type>(whiteboard);
            lastMessageSent=waiter->createWaiter<decltype(lastMessageSent)::element_type>(whiteboard);
        }

        while (true)
        {
            bool timeout=false;
            {
                auto delay=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::HeartbeatInterval>();
                if (delay!=nullptr)
                {
                    timeout=!waiter->wait(*delay);
                }
                else
                {
                    waiter->wait();
                }
            }

            lastMessageSent->has();
            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                if (timeout)
                {
                    auto message=std::make_unique<Steam::CMsgClientHeartBeatMessageType>();
                    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
                }
            }
        }
    });
}
