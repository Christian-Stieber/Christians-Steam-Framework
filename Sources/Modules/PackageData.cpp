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

/************************************************************************/
/*
 * This manages "package data"
 *
 * Eventually, this will be
 *  - shared between clients
 *  - kept in a data file
 */

#include "Client/Module.hpp"
#include "Modules/Connection.hpp"
#include "Modules/PackageData.hpp"
#include "Client/DataFile.hpp"
#include "Steam/KeyValue.hpp"
#include "JobID.hpp"
#include "Vector.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_appinfo.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier LicenseIdentifier;
typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;
typedef SteamBot::Modules::PackageData::PackageInfo PackageInfo;

/************************************************************************/

namespace
{
    class PackageData : public SteamBot::Printable
    {
    private:
        SteamBot::DataFile file{"PackageData", SteamBot::DataFile::FileType::Steam};

    private:
        mutable boost::fibers::mutex mutex;
        std::unordered_map<SteamBot::PackageID, std::shared_ptr<const PackageInfo>> data;
        std::unordered_map<SteamBot::AppID, std::vector<SteamBot::PackageID>> appData;
        bool wasUpdated=false;

        std::unordered_map<SteamBot::JobID, Licenses::Ptr> updates;

    private:
        boost::json::value toJson_noMutex() const;

    private:
        PackageData();
        virtual ~PackageData();
        virtual boost::json::value toJson() const;

    public:
        static PackageData& get();

        std::vector<std::shared_ptr<const Licenses::LicenseInfo>> checkForUpdates(const Licenses&) const;
        void update(const ::CMsgClientPICSProductInfoResponse_PackageInfo&);
        void storeNew_noLock(std::shared_ptr<PackageInfo>);

        std::shared_ptr<const PackageInfo> lookup(const LicenseIdentifier&) const;
        std::vector<std::shared_ptr<const PackageInfo>> lookup(SteamBot::AppID) const;

        void save();
    };
};

/************************************************************************/

namespace
{
    class PackageDataModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<Licenses::Ptr> licensesWaiter;

        SteamBot::JobID latestJobId{0};
        Licenses::Ptr latestLicenses;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType>) const;
        void handle(Licenses::Ptr);

    public:
        PackageDataModule();
        virtual ~PackageDataModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    PackageDataModule::Init<PackageDataModule> init;
}

/************************************************************************/

static void iterateOverAppids(const PackageInfo& package, std::function<void(SteamBot::AppID)> function)
{
    if (auto appidsValue=package.data.if_contains("appids"))
    {
        if (auto appids=appidsValue->if_object())
        {
            for (const auto& item : *appids)
            {
                // ToDo: does the name actually mean something?
                // This was a KeyValue, which doesn't have arrays -- but that
                // doesn't mean the name can't have any meaning

                auto appId=item.value().to_number<std::underlying_type_t<SteamBot::AppID>>();
                function(static_cast<SteamBot::AppID>(appId));
            }
        }
    }
}

/************************************************************************/

void PackageData::storeNew_noLock(std::shared_ptr<PackageInfo> packageInfo)
{
    // If we have a previous item, unlink it from the apps
    {
        auto iterator=data.find(packageInfo->packageId);
        if (iterator!=data.end())
        {
            iterateOverAppids(*(iterator->second), [this, packageId=packageInfo->packageId](SteamBot::AppID appId) {
                auto iterator=appData.find(appId);
                if (iterator!=appData.end())
                {
                    auto count=SteamBot::erase(iterator->second, [packageId](SteamBot::PackageID item) { return item==packageId; });
                    assert(count==0 || count==1);
                }
            });
        }
    }

    // Link the new package to the apps
    {
        iterateOverAppids(*packageInfo, [this, packageId=packageInfo->packageId](SteamBot::AppID appId) {
            auto& items=appData[appId];
            for (SteamBot::PackageID item : items)
            {
                if (item==packageId)
                {
                    return;
                }
            }
            items.push_back(packageId);
        });
    }
    data[packageInfo->packageId]=std::move(packageInfo);
}

/************************************************************************/

PackageData::PackageData()
{
    file.examine([this](const boost::json::value& json) {
        for (const auto& item : json.as_object())
        {
            auto packageInfo=std::make_shared<PackageInfo>(item.value());
            storeNew_noLock(std::move(packageInfo));
        }
    });
    BOOST_LOG_TRIVIAL(debug) << "loaded packageData from file: " << *this;
}

/************************************************************************/

PackageData::~PackageData()
{
    assert(false);
}

/************************************************************************/

PackageInfo::~PackageInfo() =default;

/************************************************************************/

PackageInfo::PackageInfo(SteamBot::PackageID packageId_)
    : LicenseIdentifier(packageId_)
{
}

/************************************************************************/

static const char data_key[]="data";

/************************************************************************/
/*
 * Make sure this matches the toJson()
 */

PackageInfo::PackageInfo(const boost::json::value& json)
    : LicenseIdentifier(json)
{
    data=json.at(data_key).as_object();
}

/************************************************************************/
/*
 * Make sure this matches the json-based constructor
 */

boost::json::value PackageInfo::toJson() const
{
    auto parent=LicenseIdentifier::toJson();
    auto& json=parent.as_object();
    json[data_key]=data;
    return json;
}

/************************************************************************/

boost::json::value PackageData::toJson_noMutex() const
{
    boost::json::object json;
    for (const auto& item : data)
    {
        boost::json::value child;
        if (item.second)
        {
            child=item.second->toJson();
        }
        json[std::to_string(static_cast<std::underlying_type_t<decltype(item.first)>>(item.first))]=std::move(child);
    }
    return json;
}

/************************************************************************/

boost::json::value PackageData::toJson() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return toJson_noMutex();
}

/************************************************************************/

PackageDataModule::PackageDataModule()
{
    // This causes the instance to be created, and the data to be
    // loaded.  So, any major problems are happening at startup, not
    // at some random later occasion.
    PackageData::get();

    auto& client=getClient();
    licensesWaiter=client.whiteboard.createWaiter<Licenses::Ptr>(*waiter);
}

/************************************************************************/

PackageData& PackageData::get()
{
    static PackageData& packageData=*new PackageData();
    return packageData;
}

/************************************************************************/

std::vector<std::shared_ptr<const Licenses::LicenseInfo>> PackageData::checkForUpdates(const Licenses& licenses) const
{
    std::vector<std::shared_ptr<const Licenses::LicenseInfo>> updateList;

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        for (const auto& item : licenses.licenses)
        {
            const auto& license=*(item.second);

            bool needsUpdate=true;
            {
                auto iterator=data.find(license.packageId);
                if (iterator!=data.end())
                {
                    if (static_cast<const LicenseIdentifier&>(license)==static_cast<const LicenseIdentifier&>(*(iterator->second)))
                    {
                        needsUpdate=false;
                    }
                }
            }

            if (needsUpdate)
            {
                updateList.emplace_back(item.second);
            }
        }
    }

    {
        boost::json::array json;
        for (const auto& licenseInfo : updateList)
        {
            json.emplace_back(licenseInfo->toJson());
        }
        BOOST_LOG_TRIVIAL(info) << "packageData needs to be updated for: " << json;
    }

    return updateList;
}

/************************************************************************/
/*
 * Update our database with the new data
 */

void PackageData::update(const ::CMsgClientPICSProductInfoResponse_PackageInfo& package)
{
    if (package.has_packageid())
    {
        auto packageInfo=std::make_shared<PackageInfo>(static_cast<SteamBot::PackageID>(package.packageid()));

        if (package.has_change_number())
        {
            packageInfo->changeNumber=package.change_number();
        }

        if (package.has_buffer())
        {
            Steam::KeyValue::BinaryDeserializationType bytes(static_cast<const std::byte*>(static_cast<const void*>(package.buffer().data())), package.buffer().size());

            // SteamKit has this to say:
            // steamclient checks this value == 1 before it attempts to read the KV from the buffer
            // see: CPackageInfo::UpdateFromBuffer(CSHA const&,uint,CUtlBuffer &)
            // todo: we've apparently ignored this with zero ill effects, but perhaps we want to respect it?
            if (bytes.size()>=4)
            {
                bytes=bytes.last(bytes.size()-4);
                std::string name;
                if (auto tree=Steam::KeyValue::deserialize(bytes, name))
                {
                    packageInfo->data=tree->toJson().as_object();
                }
            }
        }

        std::lock_guard<decltype(mutex)> lock(mutex);

        // Verify: we are assuming that changeNumber increases
        auto iterator=data.find(packageInfo->packageId);
        if (iterator==data.end() || iterator->second->changeNumber<packageInfo->changeNumber)
        {
            storeNew_noLock(std::move(packageInfo));
            wasUpdated=true;
        }
    }
}

/************************************************************************/
/*
 * ToDo: we should offload this to a thread, and use a timer
 * to only save if there haven't been updates for some time.
 *
 * But, for now, this works too. And it's much simpler.
 */

void PackageData::save()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (wasUpdated)
    {
        file.update([this](boost::json::value& fileData) {
            fileData=toJson_noMutex();
        });
        wasUpdated=false;
    }
}

/************************************************************************/

void PackageDataModule::handle(Licenses::Ptr licenses)
{
    auto updateList=PackageData::get().checkForUpdates(*licenses);

    if (!updateList.empty())
    {
        auto message=std::make_unique<Steam::CMsgClientPICSProductInfoRequestMessageType>();
        for (const auto& licenseInfo : updateList)
        {
            auto& package=*(message->content.add_packages());
            package.set_packageid(static_cast<std::underlying_type_t<decltype(licenseInfo->packageId)>>(licenseInfo->packageId));
            package.set_access_token(licenseInfo->accessToken);
        }

        latestLicenses=licenses;
        latestJobId=SteamBot::JobID();
        message->header.proto.set_jobid_source(latestJobId.getValue());

        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
    }
}

/************************************************************************/

SteamBot::Modules::PackageData::Whiteboard::PackageData::PackageData(Licenses::Ptr licenses_)
    : licenses(std::move(licenses_))
{
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType> message) const
{
    for (int i=0; i<message->content.packages_size(); i++)
    {
        PackageData::get().update(message->content.packages(i));
    }
    PackageData::get().save();

    if (message->header.proto.jobid_target()==latestJobId.getValue())
    {
        getClient().whiteboard.set<SteamBot::Modules::PackageData::Whiteboard::PackageData>(latestLicenses);
    }
}

/************************************************************************/

void PackageDataModule::run(SteamBot::Client& client)
{
    auto cmsgClientPICSProductInfoResponse=client.messageboard.createWaiter<Steam::CMsgClientPICSProductInfoResponseMessageType>(*waiter);

    while (true)
    {
        waiter->wait();
        if (licensesWaiter->isWoken())
        {
            if (auto licenses=licensesWaiter->has())
            {
                handle(*licenses);
            }
        }
        cmsgClientPICSProductInfoResponse->handle(this);
    }
}

/************************************************************************/
/*
 * Only finds a package if our changeNumber no less than what
 * the license has. It will find a newer one, as an update
 * would never happen.
 *
 * Verify: Yes, I'm assuming they increase.
 */

std::shared_ptr<const PackageInfo> PackageData::lookup(const LicenseIdentifier& license) const
{
    std::shared_ptr<const PackageInfo> result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto iterator=data.find(license.packageId);
        if (iterator!=data.end())
        {
            if (iterator->second->changeNumber>=license.changeNumber)
            {
                result=iterator->second;
            }
        }
    }
    return result;
}

/************************************************************************/

std::vector<std::shared_ptr<const PackageInfo>> PackageData::lookup(SteamBot::AppID appId) const
{
    std::vector<std::shared_ptr<const PackageInfo>> result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto appIterator=appData.find(appId);
        if (appIterator!=appData.end())
        {
            result.reserve(appIterator->second.size());
            for (SteamBot::PackageID packageId : appIterator->second)
            {
                auto packageIterator=data.find(packageId);
                if (packageIterator!=data.end())
                {
                    result.emplace_back(packageIterator->second);
                }
            }
        }
    }
    return result;
}

/************************************************************************/

std::shared_ptr<const PackageInfo> SteamBot::Modules::PackageData::getPackageInfo(const LicenseIdentifier& license)
{
    return ::PackageData::get().lookup(license);
}

/************************************************************************/

std::vector<std::shared_ptr<const PackageInfo>> SteamBot::Modules::PackageData::getPackageInfo(SteamBot::AppID appId)
{
    return ::PackageData::get().lookup(appId);
}
