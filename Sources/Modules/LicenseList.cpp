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
#include "Helpers/Time.hpp"

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
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientLicenseListMessageType> cmsgClientLicenseList;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType>);

    public:
        LicenseListModule() =default;
        virtual ~LicenseListModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    LicenseListModule::Init<LicenseListModule> init;
}

/************************************************************************/

Licenses::Licenses() =default;
Licenses::~Licenses() =default;

Licenses::LicenseInfo::~LicenseInfo() =default;

/************************************************************************/

Licenses::LicenseInfo::LicenseInfo(const CMsgClientLicenseList_License& data)
    : LicenseIdentifier(static_cast<SteamBot::PackageID>(data.package_id()))
{
    if (data.has_change_number())
        changeNumber=data.change_number();

    if (data.has_license_type())
        licenseType=static_cast<SteamBot::LicenseType>(data.license_type());

    if (data.has_payment_method())
        paymentMethod=static_cast<SteamBot::PaymentMethod>(data.payment_method());

    if (data.has_access_token())
        accessToken=data.access_token();

    if (data.has_time_created())
        timeCreated=Licenses::LicenseInfo::Clock::from_time_t(data.time_created());

    if (data.has_time_next_process())
        timeNextProcess=Licenses::LicenseInfo::Clock::from_time_t(data.time_next_process());
}

/************************************************************************/

std::shared_ptr<const Licenses::LicenseInfo> Licenses::getInfo(PackageID packageId) const
{
    std::shared_ptr<const Licenses::LicenseInfo> result;
    auto iterator=licenses.find(packageId);
    if (iterator!=licenses.end())
    {
        result=iterator->second;
    }
    return result;
}

/************************************************************************/

std::shared_ptr<const Licenses::LicenseInfo> SteamBot::Modules::LicenseList::getLicenseInfo(PackageID packageId)
{
    std::shared_ptr<const Licenses::LicenseInfo> result;
    if (auto licenses=SteamBot::Client::getClient().whiteboard.has<Licenses::Ptr>())
    {
        result=(*licenses)->getInfo(packageId);
    }
    return result;
}

/************************************************************************/

static void timeToJson(boost::json::object& json, std::string_view key, const Licenses::LicenseInfo::Clock::time_point& time)
{
    if (time.time_since_epoch().count()!=0)
    {
        json[key]=SteamBot::Time::toString(time, true);
    }
}

/************************************************************************/
/*
 * Not meant to be read by anything other than human eyes.
 */

boost::json::value Licenses::LicenseInfo::toJson() const
{
    auto parent=LicenseIdentifier::toJson();
    auto& json=parent.as_object();
    SteamBot::enumToJson(json, "licenseType", licenseType);
    SteamBot::enumToJson(json, "paymentMethod", paymentMethod);
    if (accessToken!=0) json["access token"]=changeNumber;
    timeToJson(json, "timeCreated", timeCreated);
    timeToJson(json, "timeNextProcess", timeNextProcess);
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

void LicenseListModule::handle(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType> message)
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
                auto license=std::make_shared<Licenses::LicenseInfo>(licenseData);
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

void LicenseListModule::init(SteamBot::Client& client)
{
    cmsgClientLicenseList=client.messageboard.createWaiter<Steam::CMsgClientLicenseListMessageType>(*waiter);
}

/************************************************************************/

void LicenseListModule::run(SteamBot::Client& client)
{
    while (true)
    {
        waiter->wait();
        cmsgClientLicenseList->handle(this);
    }
}
