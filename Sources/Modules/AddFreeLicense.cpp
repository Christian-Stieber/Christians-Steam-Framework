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
#include "UI/UI.hpp"
#include "ResultCode.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_2.hpp"

/************************************************************************/

typedef SteamBot::Modules::AddFreeLicense::Messageboard::AddLicense AddLicense;
typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;

/************************************************************************/

namespace
{
    class AddFreeLicenseModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientRequestFreeLicenseResponseMessageType> cmsgClientRequestFreeLicenseResponse;
        SteamBot::Messageboard::WaiterType<AddLicense> addLicense;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientRequestFreeLicenseResponseMessageType>);
        void handle(std::shared_ptr<const AddLicense>);

    public:
        AddFreeLicenseModule() =default;
        virtual ~AddFreeLicenseModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    AddFreeLicenseModule::Init<AddFreeLicenseModule> init;
}

/************************************************************************/

void AddFreeLicenseModule::handle(std::shared_ptr<const Steam::CMsgClientRequestFreeLicenseResponseMessageType> message)
{
    SteamBot::UI::OutputText output;

    auto eResult=static_cast<SteamBot::ResultCode>(message->content.eresult());
    output << "license request result: " << SteamBot::enumToString(eResult);

    for (int i=0; i<message->content.granted_packageids_size(); i++)
    {
        if (i==0)
        {
            if (message->content.granted_packageids_size()>1)
            {
                output << "; package ids ";
            }
            else
            {
                output << "; package id ";
            }
        }
        else
        {
            output << ", ";
        }
        output << message->content.granted_packageids(i);
    }

    for (int i=0; i<message->content.granted_appids_size(); i++)
    {
        if (i==0)
        {
            if (message->content.granted_appids_size()>1)
            {
                output << "; app ids ";
            }
            else
            {
                output << "; app id ";
            }
        }
        else
        {
            output << ", ";
        }
        output << message->content.granted_appids(i);
    }
}

/************************************************************************/

void AddFreeLicenseModule::handle(std::shared_ptr<const AddLicense> message)
{
    auto request=std::make_unique<Steam::CMsgClientRequestFreeLicenseMessageType>();

    const auto appId=toUnsignedInteger(message->appId);
    request->content.add_appids(appId);
    SteamBot::UI::OutputText() << "requesting a free license for app id " << appId;

    SendSteamMessage::send(std::move(request));
}

/************************************************************************/

void AddFreeLicenseModule::init(SteamBot::Client& client)
{
    cmsgClientRequestFreeLicenseResponse=client.messageboard.createWaiter<Steam::CMsgClientRequestFreeLicenseResponseMessageType>(*waiter);
    addLicense=client.messageboard.createWaiter<AddLicense>(*waiter);
}

/************************************************************************/

void AddFreeLicenseModule::run(SteamBot::Client&)
{
    waitForLogin();

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
