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

typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;

/************************************************************************/

namespace
{
    class AddLicenseMessageBase
    {
    protected:
        AddLicenseMessageBase() =default;

    public:
        virtual ~AddLicenseMessageBase() =default;

        virtual void execute() const =0;
    };

    template <typename T> class AddLicenseMessage : public AddLicenseMessageBase
    {
    public:
        const T licenseId;

    public:
        AddLicenseMessage(T licenseId_)
            : licenseId(licenseId_)
        {
        }

        virtual ~AddLicenseMessage() =default;

    public:
        virtual void execute() const override;
    };
}

/************************************************************************/

namespace
{
    class AddFreeLicenseModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientRequestFreeLicenseResponseMessageType> cmsgClientRequestFreeLicenseResponse;
        SteamBot::Messageboard::WaiterType<AddLicenseMessageBase> addLicense;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientRequestFreeLicenseResponseMessageType>);
        void handle(std::shared_ptr<const AddLicenseMessageBase>);

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

void AddFreeLicenseModule::handle(std::shared_ptr<const AddLicenseMessageBase> message)
{
    message->execute();
}

/************************************************************************/

template <> void AddLicenseMessage<SteamBot::AppID>::execute() const
{
    auto request=std::make_unique<Steam::CMsgClientRequestFreeLicenseMessageType>();

    const auto appId=SteamBot::toUnsignedInteger(licenseId);
    request->content.add_appids(appId);
    SteamBot::UI::OutputText() << "requesting a free license for app id " << appId;

    SendSteamMessage::send(std::move(request));
}

/************************************************************************/

template <> void AddLicenseMessage<SteamBot::PackageID>::execute() const
{
    const auto packageId=SteamBot::toInteger(licenseId);
    SteamBot::UI::OutputText() << "requesting a free license for package id " << packageId;
}

/************************************************************************/

void AddFreeLicenseModule::init(SteamBot::Client& client)
{
    cmsgClientRequestFreeLicenseResponse=client.messageboard.createWaiter<Steam::CMsgClientRequestFreeLicenseResponseMessageType>(*waiter);
    addLicense=client.messageboard.createWaiter<AddLicenseMessageBase>(*waiter);
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

void SteamBot::Modules::AddFreeLicense::add(AppID appId)
{
    std::shared_ptr<AddLicenseMessageBase> message=std::make_shared<AddLicenseMessage<AppID>>(appId);
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}

/************************************************************************/

void SteamBot::Modules::AddFreeLicense::add(PackageID packageId)
{
    std::shared_ptr<AddLicenseMessageBase> message=std::make_shared<AddLicenseMessage<PackageID>>(packageId);
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}
