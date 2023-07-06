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

#include "Client/Module.hpp"
#include "Modules/Inventory.hpp"
#include "Modules/WebSession.hpp"
#include "Helpers/ParseNumber.hpp"
#include "RateLimit.hpp"
#include "MiscIDs.hpp"
#include "SteamID.hpp"
#include "UI/UI.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::Inventory::Messageboard::LoadInventory LoadInventory;

typedef SteamBot::Modules::Inventory::InventoryItem InventoryItem;
typedef SteamBot::Modules::Inventory::Whiteboard::Inventory Inventory;

/************************************************************************/

LoadInventory::LoadInventory() =default;
LoadInventory::~LoadInventory() =default;

/************************************************************************/

InventoryItem::InventoryItem() =default;
InventoryItem::~InventoryItem() =default;

/************************************************************************/

Inventory::Inventory() =default;
Inventory::~Inventory() =default;

/************************************************************************/

template <typename T> static bool optJsonStringNumber(const boost::json::object& json, std::string_view key, T& number)
{
    if (auto string=json.if_contains(key))
    {
        if (SteamBot::parseNumber(string->as_string(), number))
        {
            return true;
        }
        else
        {
            throw false;
        }
    }
    return false;
}

/************************************************************************/
/*
 * This takes a json from the inventory:
 * {
 *    "appid": 753,
 *    "classid": "5295844374",
 *    "instanceid": "3873503133",
 *
 *    "contextid": "6",
 *    "assetid": "25267868949",
 *    "amount": "1"
 * }
 */

bool InventoryItem::init(const boost::json::object& json)
{
    if (AssetKey::init(json))
    {
        try
        {
            optJsonStringNumber(json, "contextid", contextId);
            optJsonStringNumber(json, "assetid", assetId);
            optJsonStringNumber(json, "amount", amount);
            return true;
        }
        catch(...)
        {
        }
    }
    return false;
}

/************************************************************************/

boost::json::value InventoryItem::toJson() const
{
    auto json=AssetKey::toJson();
    auto& object=json.as_object();
    if (contextId!=0) object["contextId"]=contextId;
    if (assetId!=0) object["assetId"]=assetId;
    if (amount!=0) object["amount"]=amount;
    return json;
}

/************************************************************************/

namespace
{
    class InventorysModule : public SteamBot::Client::Module
    {
    private:
        static SteamBot::RateLimiter& getRateLimiter();

    private:
        SteamBot::Messageboard::WaiterType<LoadInventory> loadInventory;

    private:
        static void processInventoryPage(Inventory&, const boost::json::value&);
        static void load();

    public:
        void handle(std::shared_ptr<const LoadInventory>);

    public:
        InventorysModule() =default;
        virtual ~InventorysModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    InventorysModule::Init<InventorysModule> init;
}

/************************************************************************/

SteamBot::RateLimiter& InventorysModule::getRateLimiter()
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
 *
 * Returns the last asset id, or 0.
 */

void InventorysModule::processInventoryPage(Inventory& inventory, const boost::json::value& json)
{
    for (const auto& asset : json.at("assets").as_array())
    {
        auto item=std::make_unique<InventoryItem>();
        if (item->init(asset.as_object()))
        {
            BOOST_LOG_TRIVIAL(debug) << "inventory item: " << item->toJson();
            inventory.items.emplace_back(std::move(item));
        }
        else
        {
            BOOST_LOG_TRIVIAL(error) << "unable to process inventory item: " << asset;
        }
    }
    SteamBot::UI::OutputText() << "processed " << inventory.items.size() << " inventory items out of " << json.at("total_inventory_count").to_number<uint32_t>();
}

/************************************************************************/

void InventorysModule::load()
{
    static const auto appId=SteamBot::AppID::Steam;
    static const uint32_t contextId=6;	// Community Items

    boost::urls::url url("https://steamcommunity.com/inventory");

    {
        auto steamId=getClient().whiteboard.has<SteamBot::SteamID>();
        assert(steamId!=nullptr);
        url.segments().push_back(std::to_string(steamId->getValue()));
    }
    url.segments().push_back(std::to_string(static_cast<std::underlying_type_t<SteamBot::AppID>>(appId)));
    url.segments().push_back(std::to_string(contextId));

    url.params().set("l", "english");
    url.params().set("count", "50");

    auto inventory=std::make_shared<Inventory>();
    uint64_t startAssetId=0;

    while (true)
    {
        if (startAssetId!=0)
        {
            url.params().set("start_assetid", std::to_string(startAssetId));
        }

        std::shared_ptr<const Response> response;
        getRateLimiter().limit([&url, &response]() {
            auto request=std::make_shared<Request>();
            request->queryMaker=[&url](){
                return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
            };
            response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        });

        const auto json=SteamBot::HTTPClient::parseJson(*(response->query));

        try
        {
            processInventoryPage(*inventory, json);

            /*
             * {
             *    "assets": ...
             *    "descriptions": ...
             *
             *    "more_items":1,
             *    "last_assetid":"14070692302",
             *    "total_inventory_count":100,
             *    "success":1,
             *    "rwgrsn":-2
             * }
             */
            if (!(json.at("more_items").to_number<int>()==1 && optJsonStringNumber(json.as_object(), "last_assetid", startAssetId)))
            {
                break;
            }
        }
        catch(...)
        {
            break;
        }
    }

    SteamBot::UI::OutputText() << "inventory loaded";

    inventory->when=std::chrono::system_clock::now();
    getClient().whiteboard.set(std::move(inventory));
}

/************************************************************************/

void InventorysModule::handle(std::shared_ptr<const LoadInventory>)
{
    load();
    loadInventory->discardMessages();
}

/************************************************************************/

void InventorysModule::init(SteamBot::Client& client)
{
    loadInventory=client.messageboard.createWaiter<LoadInventory>(*waiter);
}

/************************************************************************/

void InventorysModule::run(SteamBot::Client& client)
{
    waitForLogin();
    while (true)
    {
        waiter->wait();
        loadInventory->handle(this);
    }
}
