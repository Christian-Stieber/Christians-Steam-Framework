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
 * Note: I found two services that might be inventory-related,
 * but could not get them to work for me:
 *    Econ.GetInventoryItemsWithDescriptions
 *    Inventory.GetInventory
 *
 * Thus, for now(?), we'll be using a http query.
 */

/************************************************************************/
/*
 * https://dev.doctormckay.com/topic/332-identifying-steam-items/
 */

/************************************************************************/

#include "Client/Module.hpp"
#include "Modules/Inventory.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/ClientNotification.hpp"
#include "Modules/Connection.hpp"
#include "AssetData.hpp"
#include "Helpers/ParseNumber.hpp"
#include "Helpers/JSON.hpp"
#include "Helpers/Time.hpp"
#include "RateLimit.hpp"
#include "MiscIDs.hpp"
#include "SteamID.hpp"
#include "UI/UI.hpp"
#include "Exceptions.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_2.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Inventory::ItemKey ItemKey;
typedef SteamBot::Inventory::Item Item;
typedef SteamBot::Inventory::Inventory Inventory;

typedef SteamBot::Modules::ClientNotification::Messageboard::ClientNotification ClientNotification;

/************************************************************************/

ItemKey::ItemKey() =default;
ItemKey::~ItemKey() =default;

Item::~Item() =default;
Inventory::~Inventory() =default;

/************************************************************************/

Inventory::Inventory()
    : when(std::chrono::system_clock::now())
{
}

/************************************************************************/
/*
 * This takes a json from the inventory:
 * {
 *    "appid": 753,
 *    "contextid": "6",
 *    "assetid": "25267868949",
 *    ...
 * }
 */

ItemKey::ItemKey(const boost::json::value& json)
{
    appId=SteamBot::JSON::toNumber<SteamBot::AppID>(json.at("appid"));
    contextId=SteamBot::JSON::toNumber<SteamBot::ContextID>(json.at("contextid"));
    assetId=SteamBot::JSON::toNumber<SteamBot::AssetID>(json.at("assetid"));
}

/************************************************************************/
/*
 * This takes a json from the inventory:
 * {
 *    "appid": 753,
 *    "contextid": "6",
 *    "assetid": "25267868949",
 *    ...
 *    "classid": "5295844374",
 *    "instanceid": "3873503133",
 *    ...
 *    "amount": "1",
 *    ...
 * }
 */

Item::Item(const boost::json::value& json)
    : ItemKey(json), AssetKey(json)
{
    assert(ItemKey::appId==AssetKey::appId);
    SteamBot::JSON::optNumber(json, "amount", amount);
}

/************************************************************************/

boost::json::value Item::toJson() const
{
    auto json=AssetKey::toJson();
    auto& object=json.as_object();
    if (contextId!=SteamBot::ContextID::None) object["contextId"]=toInteger(contextId);
    if (assetId!=SteamBot::AssetID::None) object["assetId"]=toInteger(assetId);
    if (amount!=0) object["amount"]=amount;
    return json;
}

/************************************************************************/

namespace
{
    class InventoryModule : public SteamBot::Client::Module
    {
    private:
        boost::fibers::mutex mutex;		// locked while we are loading the inventory

    private:
        std::chrono::system_clock::time_point lastUpdateNotification;

    private:
        static SteamBot::RateLimiter& getRateLimiter();
        static boost::urls::url makeUrl(SteamBot::AppID, SteamBot::ContextID);
        static std::shared_ptr<const Response> makeInventoryQuery(const boost::urls::url&);

    private:
        SteamBot::Messageboard::WaiterType<ClientNotification> clientNotification;
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientItemAnnouncementsMessageType> clientItemAnnouoncements;

    private:
        void updateNotification(std::chrono::system_clock::time_point);
        static void processInventoryDescriptions(const boost::json::value&);
        static void processInventoryAssets(Inventory&, const boost::json::value&);
        void load();

    public:
        void handle(std::shared_ptr<const ClientNotification>);
        void handle(std::shared_ptr<const Steam::CMsgClientItemAnnouncementsMessageType>);

    public:
        InventoryModule() =default;
        virtual ~InventoryModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;

        std::shared_ptr<const Inventory> get();
    };

    InventoryModule::Init<InventoryModule> init;
}

/************************************************************************/

SteamBot::RateLimiter& InventoryModule::getRateLimiter()
{
    static auto& instance=*new SteamBot::RateLimiter(std::chrono::seconds(30));
    return instance;
}

/************************************************************************/
/*
 * {
 *    "assets": [
 *       {
 *          "appid": 753,
 *          "contextid": "6",
 *          "assetid": "25267868949",
 *          "classid": "5295844374",
 *          "instanceid": "3873503133",
 *          "amount": "1"
 *       },
 *       ...
 *    ]
 *    ...
 * }
 */

void InventoryModule::processInventoryAssets(Inventory& inventory, const boost::json::value& json)
{
    for (const auto& asset : json.at("assets").as_array())
    {
        try
        {
            auto item=std::make_shared<Item>(asset);
            BOOST_LOG_TRIVIAL(debug) << "inventory item: " << item->toJson();
            inventory.items.emplace_back(std::move(item));
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(error) << "unable to process inventory item: " << asset;
            throw;
        }
    }
}

/************************************************************************/
/*
 * "descriptions": [
 *    {
 *       "appid": 753,
 *       "classid": "5295844374",
 *       "instanceid": "3873503133",
 *       "currency": 0,
 *       "background_color": "",
 *       ...
 *    },
 *    ...
 * ]
 *
 * Note: this is pretty much the same as the "Econ.GetAssetClassInfo#1" result,
 * except that we get it in JSON instead of protobuf.
 */

void InventoryModule::processInventoryDescriptions(const boost::json::value& json)
{
    for (const auto& description : json.at("descriptions").as_array())
    {
        SteamBot::AssetData::store(description);
    }
}

/************************************************************************/

boost::urls::url InventoryModule::makeUrl(SteamBot::AppID appId, SteamBot::ContextID contextId)
{
    boost::urls::url url("https://steamcommunity.com/inventory");

    {
        auto steamId=getClient().whiteboard.has<SteamBot::SteamID>();
        assert(steamId!=nullptr);
        url.segments().push_back(std::to_string(steamId->getValue()));
    }
    url.segments().push_back(std::to_string(toInteger(appId)));
    url.segments().push_back(std::to_string(toInteger(contextId)));

    url.params().set("l", "english");
    url.params().set("count", "1000");

    return url;
}

/************************************************************************/

std::shared_ptr<const Response> InventoryModule::makeInventoryQuery(const boost::urls::url& url)
{
    std::shared_ptr<const Response> response;
    getRateLimiter().limit([&url, &response]() {
        auto request=std::make_shared<Request>();
        request->queryMaker=[&url](){
            return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        };
        response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    });
    return response;
}

/************************************************************************/

void InventoryModule::load()
{
    static const auto appId=SteamBot::AppID::Steam;
    static const auto contextId=static_cast<SteamBot::ContextID>(6);	// Community Items

    try
    {
        auto url=makeUrl(appId, contextId);

        auto inventory=std::make_shared<Inventory>();
        uint64_t startAssetId=0;

        lastUpdateNotification=std::chrono::system_clock::time_point();

        while (true)
        {
            if (startAssetId!=0)
            {
                url.params().set("start_assetid", std::to_string(startAssetId));
            }

            auto response=makeInventoryQuery(url);
            const auto json=SteamBot::HTTPClient::parseJson(*(response->query));

            processInventoryAssets(*inventory, json);
            processInventoryDescriptions(json);

            /*
             * {
             *    ...
             *    "more_items":1,
             *    "last_assetid":"14070692302",
             *    "total_inventory_count":100,
             *    "success":1
             * }
             */

            {
                auto total=json.at("total_inventory_count").to_number<uint32_t>();
                SteamBot::UI::OutputText() << "processed " << inventory->items.size() << " inventory items out of " << total;
            }

            {
                int moreItems=0;
                SteamBot::JSON::optNumber(json, "more_items", moreItems);
                if (!moreItems) break;
            }

            startAssetId=SteamBot::JSON::toNumber<decltype(startAssetId)>(json.at("last_assetid"));
        }

        getClient().whiteboard.set<Inventory::Ptr>(std::move(inventory));
    }
    catch(const SteamBot::OperationCancelledException&)
    {
        throw;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "failed to load inventory";
        SteamBot::UI::OutputText() << "failed to load inventory";
        getClient().whiteboard.clear<Inventory::Ptr>();
    }
}

/************************************************************************/
/*
 * Note: while notifications give me an asset-id, I couldn't figure
 * out how to get the class-id from it. My only idea was loading the
 * inventory with a "filters" item, but... that didn't work.
 *
 * So, I'll just record the fact that something has been updated,
 * and reload the inventory after some grace delay.
 */

void InventoryModule::updateNotification(std::chrono::system_clock::time_point when)
{
    if (when>lastUpdateNotification)
    {
        lastUpdateNotification=when;
    }
}

/************************************************************************/

void InventoryModule::handle(std::shared_ptr<const Steam::CMsgClientItemAnnouncementsMessageType>)
{
    // ToDo: should we be using rtime32_gained ??
    updateNotification(std::chrono::system_clock::now());
}

/************************************************************************/

void InventoryModule::handle(std::shared_ptr<const ClientNotification> notification)
{
    if (notification->type==ClientNotification::Type::InventoryItem)
    {
        updateNotification(notification->timestamp);
    }
}

/************************************************************************/

void InventoryModule::init(SteamBot::Client& client)
{
    clientNotification=client.messageboard.createWaiter<ClientNotification>(*waiter);
    clientItemAnnouoncements=client.messageboard.createWaiter<Steam::CMsgClientItemAnnouncementsMessageType>(*waiter);
}

/************************************************************************/

void InventoryModule::run(SteamBot::Client& client)
{
    SteamBot::Modules::ClientNotification::use();

    waitForLogin();

    {
        auto message=std::make_unique<Steam::CMsgClientRequestItemAnnouncementsMessageType>();
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
    }

    while (true)
    {
        waiter->wait();
        clientNotification->handle(this);
        clientItemAnnouoncements->handle(this);
    }
}

/************************************************************************/

std::shared_ptr<const Inventory> InventoryModule::get()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (auto inventory=getClient().whiteboard.has<Inventory::Ptr>())
    {
        if ((*inventory)->when>=lastUpdateNotification)
        {
            return *inventory;
        }
    }
    load();
    return getClient().whiteboard.get<Inventory::Ptr>(nullptr);
}

/************************************************************************/
/*
 * Returns the current inventory, possibly loading it if necessary.
 */

std::shared_ptr<const Inventory> SteamBot::Inventory::get()
{
    return SteamBot::Client::getClient().getModule<InventoryModule>()->get();
}

/************************************************************************/
/*
 * That doesn't seem to work -- I was trying to add a "filter"
 * parameter that the protobuf has, but I'm either doing it
 * incorrectly, or it's not actually supported.
 *
 * I'm still leaving it around for a bit, even though it
 * won't be updated as other code changes so it probably
 * won't even compile anymore.
 */

#if 0
class InventoryModule::InventoryUpdate
{
private:
    class Items
    {
    public:
        SteamBot::AppID appId=SteamBot::AppID::None;
        SteamBot::ContextID contextId=SteamBot::ContextID::None;
        std::vector<SteamBot::AssetID> assetIds;
    };

private:
    std::vector<Items> items;

private:
    // ToDo: how are multiple items notified?
    void populateItems(const ClientNotification& notification)
    {
        auto& chunk=items.emplace_back();
        chunk.appId=SteamBot::JSON::toNumber<SteamBot::AppID>(notification.body.at("app_id"));
        chunk.contextId=SteamBot::JSON::toNumber<SteamBot::ContextID>(notification.body.at("context_id"));
        chunk.assetIds.push_back(SteamBot::JSON::toNumber<SteamBot::AssetID>(notification.body.at("asset_id")));
    }

private:
    void runQuery(const Items& chunk)
    {
        auto url=makeUrl(chunk.appId, chunk.contextId);

        {
            boost::json::array array;
            for (const auto& assetId : chunk.assetIds)
            {
                array.emplace_back(std::to_string(toInteger(assetId)));
            }
            assert(!array.empty());

            boost::json::object json;
            json["assetids"]=std::move(array);

            url.params().set("filters", boost::json::serialize(json));
        }

        auto response=makeInventoryQuery(url);
        const auto json=SteamBot::HTTPClient::parseJson(*(response->query));
    }

public:
    InventoryUpdate(const ClientNotification& notification)
    {
        assert(notification.type==ClientNotification::Type::InventoryItem);
        populateItems(notification);

        for (const auto& chunk : items)
        {
            runQuery(chunk);
        }
    }
};
#endif
