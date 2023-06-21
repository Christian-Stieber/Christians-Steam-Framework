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

#include "Steam/ProtoBuf/steammessages_clientserver_appinfo.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses::LicenseInfo LicenseInfo;
typedef SteamBot::Modules::PackageData::PackageInfo PackageInfo;

/************************************************************************/
/*
 * ToDo: this should be stored between runs
 */

namespace
{
    class PackageData
    {
    public:
        boost::fibers::mutex mutex;
        std::unordered_map<SteamBot::PackageID, std::shared_ptr<PackageInfo>> data;

    private:
        PackageData() =default;
        ~PackageData() =default;

    public:
        static PackageData& get();
    };
};

/************************************************************************/

namespace
{
    class PackageDataModule : public SteamBot::Client::Module
    {
    public:
        void handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType>);
        void handle(std::shared_ptr<const LicenseInfo>);

    public:
        PackageDataModule() =default;
        virtual ~PackageDataModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    PackageDataModule::Init<PackageDataModule> init;
}

/************************************************************************/

PackageData& PackageData::get()
{
    static PackageData& packageData=*new PackageData();
    return packageData;
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const LicenseInfo> license)
{
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
}

/************************************************************************/

void PackageDataModule::handle(std::shared_ptr<const Steam::CMsgClientPICSProductInfoResponseMessageType> message)
{
    for (int i=0; i<message->content.packages_size(); i++)
    {
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
    }
}

/************************************************************************/

void PackageDataModule::run(SteamBot::Client& client)
{
    auto license=client.messageboard.createWaiter<LicenseInfo>(*waiter);
    auto cmsgClientPICSProductInfoResponse=client.messageboard.createWaiter<Steam::CMsgClientPICSProductInfoResponseMessageType>(*waiter);

    while (true)
    {
        waiter->wait();
        license->handle(this);
        cmsgClientPICSProductInfoResponse->handle(this);
    }
}
