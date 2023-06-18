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

#include "Steam/ProtoBuf/steammessages_clientserver_uds.hpp"

/************************************************************************/

namespace
{
    class ClientAppListModule : public SteamBot::Client::Module
    {
    private:
        void handle(std::shared_ptr<const Steam::CMsgClientGetClientAppListMessageType>);

    public:
        ClientAppListModule() =default;
        virtual ~ClientAppListModule() =default;

        virtual void run() override;
    };

    ClientAppListModule::Init<ClientAppListModule> init;
}

/************************************************************************/

void ClientAppListModule::handle(std::shared_ptr<const Steam::CMsgClientGetClientAppListMessageType> request)
{
    auto response=std::make_unique<Steam::CMsgClientGetClientAppListResponseMessageType>();
    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(response));
}

/************************************************************************/

void ClientAppListModule::run()
{
    getClient().launchFiber("ClientAppListModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientGetClientAppListMessageType>> clientAppListQueue;
        clientAppListQueue=waiter->createWaiter<decltype(clientAppListQueue)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter->wait();
            handle(clientAppListQueue->fetch());
        }
    });
}
