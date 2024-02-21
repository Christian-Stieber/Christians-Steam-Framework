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

#include "Client/Module.hpp"
#include "Modules/InventoryNotification.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Helpers/JSON.hpp"
#include "Helpers/ProtoBuf.hpp"
#include "UI/UI.hpp"

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Econ::GetInventoryItemsWithDescriptions)> GetInventoryItemsWithDescriptionsInfo;

typedef SteamBot::Modules::ClientNotification::Messageboard::ClientNotification ClientNotification;
typedef SteamBot::Modules::InventoryNotification::Messageboard::InventoryNotification InventoryNotification;

/************************************************************************/

namespace
{
    class InventoryNotificationModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<ClientNotification> clientNotificationWaiter;

    public:
        void handle(std::shared_ptr<const ClientNotification>);

    public:
        InventoryNotificationModule() =default;
        virtual ~InventoryNotificationModule() =default;

        virtual void run(SteamBot::Client&) override;
        virtual void init(SteamBot::Client&) override;
    };

    InventoryNotificationModule::Init<InventoryNotificationModule> init;
}

/************************************************************************/

InventoryNotification::InventoryNotification(decltype(InventoryNotification::clientNotification) clientNotification_,
                                             decltype(InventoryNotification::itemKey) itemKey_,
                                             decltype(InventoryNotification::assetInfo) assetInfo_)
    : clientNotification(std::move(clientNotification_)), itemKey(std::move(itemKey_)), assetInfo(std::move(assetInfo_))
{
}

InventoryNotification::~InventoryNotification() =default;

/************************************************************************/

boost::json::value InventoryNotification::toJson() const
{
    boost::json::object json;
    json["itemKey"]=itemKey->toJson();
    json["description"]=assetInfo->toJson();
    return json;
}

/************************************************************************/
/*
 * ToDo: add some caching, so we don't fetch the same data again
 * ToDo: before fetching data, check whether the item is in our inventory data
 */

void InventoryNotificationModule::handle(std::shared_ptr<const ClientNotification> notification)
{
    if (!notification->read && notification->type==ClientNotification::Type::InventoryItem)
    {
        BOOST_LOG_TRIVIAL(debug) << "client notification body: " << notification->body;

        auto& client=SteamBot::Client::getClient();

        auto itemKey=std::make_shared<SteamBot::Inventory::ItemKey>(notification->body);

        std::shared_ptr<GetInventoryItemsWithDescriptionsInfo::ResultType> response;
        {
            GetInventoryItemsWithDescriptionsInfo::RequestType request;
            {
                auto steamId=client.whiteboard.get<SteamBot::Modules::Login::Whiteboard::SteamID>();
                request.set_steamid(steamId.getValue());
            }
            request.set_language("english");
            request.set_appid(static_cast<uint32_t>(SteamBot::toInteger(itemKey->appId)));
            request.set_contextid(static_cast<uint32_t>(SteamBot::toInteger(itemKey->contextId)));
            request.set_get_descriptions(true);
            auto filters=request.mutable_filters();
            filters->add_assetids(static_cast<uint64_t>(SteamBot::toInteger(itemKey->assetId)));

            response=SteamBot::Modules::UnifiedMessageClient::execute<GetInventoryItemsWithDescriptionsInfo::ResultType>("Econ.GetInventoryItemsWithDescriptions#1", std::move(request));
        }

        assert(response->assets_size()==1);
        assert(response->descriptions_size()==1);

        std::shared_ptr<const SteamBot::AssetData::AssetInfo> info;
        {
            auto json=SteamBot::toJson(dynamic_cast<const google::protobuf::Message&>(response->descriptions(0)));
            auto assetKey=std::make_shared<SteamBot::AssetKey>(json);
            info=SteamBot::AssetData::query(assetKey);
            if (!info)
            {
                info=SteamBot::AssetData::store(json);
                if (!info)
                {
                    info=SteamBot::AssetData::query(assetKey);
                }
            }
        }

        if (info)
        {
            auto inventoryNotification=std::make_shared<InventoryNotification>(std::move(notification), std::move(itemKey), std::move(info));
            SteamBot::UI::OutputText() << "inventory notification: " << inventoryNotification->toJson();
            client.messageboard.send(std::move(inventoryNotification));
        }
    }
}

/************************************************************************/

void InventoryNotificationModule::init(SteamBot::Client& client)
{
    clientNotificationWaiter=client.messageboard.createWaiter<ClientNotification>(*waiter);
}

/************************************************************************/

void InventoryNotificationModule::run(SteamBot::Client&)
{
    while (true)
    {
        waiter->wait();
        clientNotificationWaiter->handle(this);
    }
}

/************************************************************************/
/*
 * Just using this to pull in the module whenever we register it
 * with the messageboard
 */

void InventoryNotification::createdWaiter()
{
}
