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

typedef SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier LicenseIdentifier;
typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

/************************************************************************/

LicenseIdentifier::LicenseIdentifier(PackageID packageId_)
    : packageId(packageId_)
{
}

LicenseIdentifier::LicenseIdentifier(const LicenseIdentifier&) =default;
LicenseIdentifier::~LicenseIdentifier() =default;
LicenseIdentifier& LicenseIdentifier::operator=(const LicenseIdentifier&) =default;

/************************************************************************/

static const char packageId_key[]="packageId";
static const char changeNumber_key[]="changeNumber";

/************************************************************************/
/*
 * Make sure this matches the json-based constructtor
 */

boost::json::value LicenseIdentifier::toJson() const
{
    boost::json::object json;
    json[packageId_key]=static_cast<std::underlying_type_t<decltype(packageId)>>(packageId);
    json[changeNumber_key]=changeNumber;
    return json;
}

/************************************************************************/
/*
 * Make sure this matches the toJson()
 */

LicenseIdentifier::LicenseIdentifier(const boost::json::value& json)
{
    packageId=static_cast<SteamBot::PackageID>(json.at(packageId_key).to_number<std::underlying_type_t<SteamBot::PackageID>>());
    changeNumber=json.at(changeNumber_key).to_number<decltype(changeNumber)>();
}

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

Licenses::LicenseInfo::~LicenseInfo() =default;

/************************************************************************/

const Licenses::LicenseInfo* Licenses::getInfo(PackageID packageId) const
{
    auto iterator=licenses.find(packageId);
    if (iterator!=licenses.end())
    {
        return iterator->second.get();
    }
    return nullptr;
}

/************************************************************************/

boost::json::value Licenses::LicenseInfo::toJson() const
{
    auto parent=LicenseIdentifier::toJson();
    auto& json=parent.as_object();
    SteamBot::enumToJson(json, "licenseType", licenseType);
    SteamBot::enumToJson(json, "paymentMethod", paymentMethod);
    if (accessToken!=0) json["access token"]=changeNumber;
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
    auto& client=getClient();

    auto licenses=std::make_shared<Licenses>();

    auto existingLicenses=client.whiteboard.get<Licenses::Ptr>(nullptr);

    for (int index=0; index<message->content.licenses_size(); index++)
    {
        const auto& licenseData=message->content.licenses(index);
        if (licenseData.has_package_id())
        {
            const auto packageId=static_cast<SteamBot::PackageID>(licenseData.package_id());
            if (packageId!=SteamBot::PackageID::Steam)		// we don't need this
            {
                auto license=std::make_shared<Licenses::LicenseInfo>(packageId);
                {
                    if (licenseData.has_change_number()) license->changeNumber=licenseData.change_number();
                    if (licenseData.has_license_type()) license->licenseType=static_cast<SteamBot::LicenseType>(licenseData.license_type());
                    if (licenseData.has_payment_method()) license->paymentMethod=static_cast<SteamBot::PaymentMethod>(licenseData.payment_method());
                    if (licenseData.has_access_token()) license->accessToken=licenseData.access_token();
                }

                bool success=licenses->licenses.try_emplace(license->packageId, std::move(license)).second;
                assert(success);
            }
        }
    }

    BOOST_LOG_TRIVIAL(info) << "license list: " << *licenses;
    SteamBot::UI::OutputText() << "account has " << licenses->licenses.size() << " licenses";

    client.whiteboard.set<Licenses::Ptr>(std::move(licenses));
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
