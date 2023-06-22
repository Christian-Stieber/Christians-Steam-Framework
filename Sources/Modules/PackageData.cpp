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

#include "Steam/ProtoBuf/steammessages_clientserver_appinfo.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier LicenseIdentifier;
typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;
typedef SteamBot::Modules::PackageData::PackageInfo PackageInfo;
typedef SteamBot::Modules::PackageData::PackageInfoFull PackageInfoFull;

/************************************************************************/
/*
 * ToDo: this should be stored between runs
 */

namespace
{
    class PackageData : public SteamBot::Printable
    {
    private:
        SteamBot::DataFile file{"PackageData", SteamBot::DataFile::FileType::Steam};

    public:
        mutable boost::fibers::mutex mutex;
        std::unordered_map<SteamBot::PackageID, std::shared_ptr<const PackageInfo>> data;
        bool wasUpdated=false;

    private:
        boost::json::value toJson_noMutex() const;

    private:
        PackageData();
        virtual ~PackageData();
        virtual boost::json::value toJson() const;

    public:
        static PackageData& get();

        std::vector<std::shared_ptr<const Licenses::LicenseInfo>> flagForUpdates(const Licenses&);
        void update(const ::CMsgClientPICSProductInfoResponse_PackageInfo&);

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

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType>);
        void handle(const Licenses&);

    public:
        PackageDataModule();
        virtual ~PackageDataModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    PackageDataModule::Init<PackageDataModule> init;
}

/************************************************************************/

PackageData::PackageData()
{
    file.examine([this](const boost::json::value& json) {
        for (const auto& item : json.as_object())
        {
            auto packageInfo=std::make_shared<PackageInfoFull>(item.value());
            data[packageInfo->packageId]=std::move(packageInfo);
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
PackageInfoFull::~PackageInfoFull() =default;

/************************************************************************/

PackageInfo::PackageInfo(const LicenseIdentifier& other)
    : LicenseIdentifier(other)
{
}

/************************************************************************/

static const char data_key[]="data";

/************************************************************************/
/*
 * Make sure this matches the toJson()
 */

PackageInfoFull::PackageInfoFull(const boost::json::value& json)
    : PackageInfo(json)
{
    data=json.at(data_key).as_object();
}

/************************************************************************/
/*
 * Make sure this matches the json-based constructor
 */

boost::json::value PackageInfoFull::toJson() const
{
    auto parent=PackageInfo::toJson();
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
/*
 * Go through the list of licenses, find out which ones have changed
 * (including the ones we don't have data for yet). Mark them as
 * updating in the database, and return a list of them.
 */

std::vector<std::shared_ptr<const Licenses::LicenseInfo>> PackageData::flagForUpdates(const Licenses& licenses)
{
    std::vector<std::shared_ptr<const Licenses::LicenseInfo>> updateList;

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        for (const auto& item : licenses.licenses)
        {
            const auto& license=*(item.second);

            auto& packageInfo=data[license.packageId];
            if (packageInfo)
            {
                if (static_cast<const LicenseIdentifier&>(license)!=static_cast<const LicenseIdentifier&>(*packageInfo))
                {
                    packageInfo.reset();
                }
            }
            if (!packageInfo)
            {
                packageInfo=std::make_shared<PackageInfo>(license);
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
        auto packageId=static_cast<SteamBot::PackageID>(package.packageid());
        auto packageInfo=std::make_shared<PackageInfoFull>(packageId);

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
        data[packageId]=std::move(packageInfo);
        wasUpdated=true;
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

void PackageDataModule::handle(const Licenses& licenses)
{
    auto updateList=PackageData::get().flagForUpdates(licenses);

    if (!updateList.empty())
    {
        auto message=std::make_unique<Steam::CMsgClientPICSProductInfoRequestMessageType>();
        for (const auto& licenseInfo : updateList)
        {
            auto& package=*(message->content.add_packages());
            package.set_packageid(static_cast<std::underlying_type_t<decltype(licenseInfo->packageId)>>(licenseInfo->packageId));
            package.set_access_token(licenseInfo->accessToken);
        }
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
    }
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType> message)
{
    for (int i=0; i<message->content.packages_size(); i++)
    {
        PackageData::get().update(message->content.packages(i));
    }
    PackageData::get().save();
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
                handle(**licenses);
            }
        }
        cmsgClientPICSProductInfoResponse->handle(this);
    }
}
