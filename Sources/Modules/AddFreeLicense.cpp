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
#include "Modules/AddFreeLicense.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_2.hpp"

/************************************************************************/

typedef SteamBot::Modules::AddFreeLicense::Messageboard::AddLicense AddLicense;
typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;

/************************************************************************/

namespace
{
    class AddFreeLicenseModule : public SteamBot::Client::Module
    {
    public:
        void handle(std::shared_ptr<const Steam::CMsgClientRequestFreeLicenseResponseMessageType>);
        void handle(std::shared_ptr<const AddLicense>);

    public:
        AddFreeLicenseModule() =default;
        virtual ~AddFreeLicenseModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    AddFreeLicenseModule::Init<AddFreeLicenseModule> init;
}

/************************************************************************/

void AddFreeLicenseModule::handle(std::shared_ptr<const Steam::CMsgClientRequestFreeLicenseResponseMessageType> message)
{
}

/************************************************************************/

void AddFreeLicenseModule::handle(std::shared_ptr<const AddLicense> message)
{
    auto request=std::make_unique<Steam::CMsgClientRequestFreeLicenseMessageType>();
    {
        request->content.add_appids(static_cast<std::underlying_type_t<decltype(message->appId)>>(message->appId));
    }
    SendSteamMessage::send(std::move(request));
}

/************************************************************************/

void AddFreeLicenseModule::run(SteamBot::Client& client)
{
    typedef SteamBot::Messageboard::Waiter<Steam::CMsgClientRequestFreeLicenseResponseMessageType> CMsgClientRequestFreeLicenseResponseWaiterType;
    auto cmsgClientRequestFreeLicenseResponse=waiter->createWaiter<CMsgClientRequestFreeLicenseResponseWaiterType>(client.messageboard);
    auto addLicense=waiter->createWaiter<SteamBot::Messageboard::Waiter<AddLicense>>(client.messageboard);

    while (true)
    {
        waiter->wait();
        cmsgClientRequestFreeLicenseResponse->handle(this);
        addLicense->handle(this);
    }
}

/************************************************************************/

void AddLicense::add(AppID appId)
{
    SteamBot::Client::getClient().messageboard.send(std::make_shared<AddLicense>(appId));
}