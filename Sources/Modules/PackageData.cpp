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
    class PackageData
    {
    private:
        SteamBot::DataFile file{"PackageData", SteamBot::DataFile::FileType::Steam};

    public:
        boost::fibers::mutex mutex;
        std::unordered_map<SteamBot::PackageID, std::shared_ptr<const PackageInfo>> data;

    private:
        PackageData();
        ~PackageData() =delete;

    public:
        static PackageData& get();

        std::vector<std::shared_ptr<const Licenses::LicenseInfo>> flagForUpdates(const Licenses&);
    };
};

/************************************************************************/

namespace
{
    class PackageDataModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<Licenses> licensesWaiter;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType>);
        void handle(std::shared_ptr<const Licenses>);

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
    });
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

boost::json::value PackageInfoFull::toJson() const
{
    auto parent=PackageInfo::toJson();
    auto& json=parent.as_object();
    json["data"]=data.toJson();
    return json;
}

/************************************************************************/

PackageDataModule::PackageDataModule()
{
    auto& client=getClient();
    licensesWaiter=client.messageboard.createWaiter<Licenses>(*waiter);
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

    return updateList;
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const Licenses> licenses)
{
    auto updateList=PackageData::get().flagForUpdates(*licenses);






#if 0
    static bool done=false;

    if (!done && license->packageId==static_cast<SteamBot::PackageID>(130344))
    {
        done=true;
        auto message=std::make_unique<Steam::CMsgClientPICSProductInfoRequestMessageType>();
        auto& info=*(message->content.add_packages());
        info.set_packageid(static_cast<std::underlying_type_t<decltype(license->packageId)>>(license->packageId));
        info.set_access_token(license->accessToken);
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
    }
#endif
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType> message)
{
    for (int i=0; i<message->content.packages_size(); i++)
    {
#if 0
        auto packageData=std::make_unique<PackageInfo::Data>();

        const auto& package=message->content.packages(i);
        if (package.has_change_number())
        {
            packageData->changeNumber=package.change_number();
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
                    packageData->data=std::move(*(tree.release()));

                    // This is an example kvTree (named with the packageId):
                    // {"appitems": {},
                    //  "depotids": { "1":311732, "0":311731 },
                    //  "appids": { "0":311730 },
                    //  "extended": { "allowcrossregiontradingandgifting":"false" },
                    //  "status": 0,
                    //  "licensetype": 1,
                    //  "billingtype": 12,
                    //  "packageid": 130344
                    // }
                }
            }
        }
#endif
    }
}

/************************************************************************/

void PackageDataModule::run(SteamBot::Client& client)
{
    auto cmsgClientPICSProductInfoResponse=client.messageboard.createWaiter<Steam::CMsgClientPICSProductInfoResponseMessageType>(*waiter);

    while (true)
    {
        waiter->wait();
        licensesWaiter->handle(this);
        cmsgClientPICSProductInfoResponse->handle(this);
    }
}
