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
#include "Modules/WebSession.hpp"
#include "Client/Module.hpp"
#include "Modules/AddFreeLicense.hpp"
#include "Modules/LicenseList.hpp"
#include "UI/UI.hpp"
#include "ResultCode.hpp"
#include "Vector.hpp"
#include "EnumString.hpp"
#include "Helpers/Time.hpp"

#include <boost/url/url_view.hpp>

#include "Steam/ProtoBuf/steammessages_clientserver_2.hpp"

/************************************************************************/

typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;
typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

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
        SteamBot::Whiteboard::WaiterType<Licenses::Ptr> licensesWaiter;
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientRequestFreeLicenseResponseMessageType> cmsgClientRequestFreeLicenseResponse;
        SteamBot::Messageboard::WaiterType<AddLicenseMessageBase> addLicense;

        // Note: we should be able to do something similar for AppID
        // requests, but I'm not all that interested in those right now
        class RequestedPackage
        {
        public:
            std::chrono::system_clock::time_point when{std::chrono::system_clock::now()};
            SteamBot::PackageID packageId;

        public:
            RequestedPackage(SteamBot::PackageID packageId_)
                : packageId(packageId_)
            {
            }
        };
        std::vector<RequestedPackage> requestedPackages;

    private:
        void checkLicenses();

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
    // ToDo: this is a bit hackish, but it should do the trick for now
    if (auto packageMessage=dynamic_cast<const AddLicenseMessage<SteamBot::PackageID>*>(message.get()))
    {
        // Note: yes, I don't care about duplicates here. Basically,
        // if we request the same package twice, getting the success
        // message twice doesn't exactly feel wrong...

        requestedPackages.emplace_back(packageMessage->licenseId);
    }

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

namespace
{
    class FreeLicense : public SteamBot::Modules::WebSession::PostWithSession
    {
    public:
        FreeLicense(SteamBot::PackageID packageId)
        {
            static const boost::urls::url_view baseUrl("https://store.steampowered.com/freelicense/addfreelicense?l=english");
            url=baseUrl;

            const auto value=toInteger(packageId);
            url.segments().push_back(std::to_string(value));
            SteamBot::UI::OutputText() << "requesting a free license for package id " << value;
        }
    };
}

/************************************************************************/

template <> void AddLicenseMessage<SteamBot::PackageID>::execute() const
{
    auto response=FreeLicense(licenseId).execute();
    if (response->query->response.result()==boost::beast::http::status::ok)
    {
        // ToDo: is there useful information in the response?
#if 0
        auto data=SteamBot::HTTPClient::parseString(*(response->query));
        BOOST_LOG_TRIVIAL(debug) << data;
#endif
    }
    else
    {
        SteamBot::UI::OutputText() << "free license error";
    }
}

/************************************************************************/

void AddFreeLicenseModule::init(SteamBot::Client& client)
{
    cmsgClientRequestFreeLicenseResponse=client.messageboard.createWaiter<Steam::CMsgClientRequestFreeLicenseResponseMessageType>(*waiter);
    addLicense=client.messageboard.createWaiter<AddLicenseMessageBase>(*waiter);
    licensesWaiter=client.whiteboard.createWaiter<Licenses::Ptr>(*waiter);
}

/************************************************************************/
/*
 * Check whether we have any licenses recorded in our
 * requestedPackages list. If so, output some information and remove
 * from the requestedPackages.
 */

void AddFreeLicenseModule::checkLicenses()
{
    if (auto licenses=licensesWaiter->has())
    {
        SteamBot::erase(requestedPackages, [&licenses](const RequestedPackage& requested) {
            if (auto license=licenses->get()->getInfo(requested.packageId))
            {
                SteamBot::UI::OutputText() << "package " << SteamBot::toInteger(requested.packageId)
                                           << " created " << SteamBot::Time::toString(license->timeCreated)
                                           << "; type " << SteamBot::enumToStringAlways(license->licenseType)
                                           << "; payment " << SteamBot::enumToStringAlways(license->paymentMethod);
                return true;
            }
            return false;
        });
    }
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
        checkLicenses();
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
