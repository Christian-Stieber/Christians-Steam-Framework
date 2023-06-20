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

#include "Steam/ProtoBuf/steammessages_clientserver.hpp"

#include "Client/Module.hpp"
#include "Modules/LicenseList.hpp"
#include "EnumString.hpp"
#include "UI/UI.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

/************************************************************************/

namespace
{
    class LicenseListModule : public SteamBot::Client::Module
    {
    private:
        void handleMessage(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType>);

    public:
        LicenseListModule() =default;
        virtual ~LicenseListModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    LicenseListModule::Init<LicenseListModule> init;
}

/************************************************************************/

Licenses::Licenses() =default;
Licenses::~Licenses() =default;

Licenses::LicenseInfo::LicenseInfo() =default;
Licenses::LicenseInfo::~LicenseInfo() =default;

/************************************************************************/

boost::json::value Licenses::LicenseInfo::toJson() const
{
    boost::json::object json;
    json["packageId"]=static_cast<std::underlying_type_t<decltype(packageId)>>(packageId);
    SteamBot::enumToJson(json, "licenseType", licenseType);
    SteamBot::enumToJson(json, "paymentMethod", paymentMethod);
    return json;
}

/************************************************************************/

boost::json::value Licenses::toJson() const
{
    boost::json::array json;
    for (const auto& item : licenses)
    {
        json.push_back(item.second->toJson());
    }
    return json;
}

/************************************************************************/

void LicenseListModule::handleMessage(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType> message)
{
    Licenses licenses;

    for (int index=0; index<message->content.licenses_size(); index++)
    {
        const auto& licenseData=message->content.licenses(index);
        if (licenseData.has_package_id())
        {
            auto license=std::make_shared<Licenses::LicenseInfo>();
            license->packageId=static_cast<SteamBot::PackageID>(licenseData.package_id());
            if (licenseData.has_license_type()) license->licenseType=static_cast<SteamBot::LicenseType>(licenseData.license_type());
            if (licenseData.has_payment_method()) license->paymentMethod=static_cast<SteamBot::PaymentMethod>(licenseData.payment_method());

            bool success=licenses.licenses.try_emplace(license->packageId, std::move(license)).second;
            assert(success);
        }
    }

    BOOST_LOG_TRIVIAL(info) << "license list: " << licenses;
    SteamBot::UI::OutputText() << "account has " << licenses.licenses.size() << " licenses";

    getClient().whiteboard.set(std::move(licenses));
}

/************************************************************************/

void LicenseListModule::run(SteamBot::Client& client)
{
    std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLicenseListMessageType>> cmsgClientLicenseList;
    cmsgClientLicenseList=waiter->createWaiter<decltype(cmsgClientLicenseList)::element_type>(client.messageboard);

    while (true)
    {
        waiter->wait();
        handleMessage(cmsgClientLicenseList->fetch());
    }
}
