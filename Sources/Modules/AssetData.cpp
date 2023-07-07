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

#include "Client/Module.hpp"
#include "DerefStuff.hpp"
#include "Modules/AssetData.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Helpers/JSON.hpp"
#include "Helpers/ProtoBuf.hpp"
#include "UI/UI.hpp"

#include <unordered_set>

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::TradeOffers::Messageboard::IncomingTradeOffers IncomingTradeOffers;
typedef SteamBot::Modules::AssetData::Messageboard::IncomingTradeOffers MyIncomingTradeOffers;

/************************************************************************/

typedef SteamBot::Modules::AssetData::AssetInfo AssetInfo;

typedef std::shared_ptr<AssetInfo> AssetInfoPtr;
typedef std::shared_ptr<SteamBot::AssetKey> AssetKeyPtr;

typedef std::unordered_set<AssetKeyPtr, SteamBot::SmartDerefStuff<AssetKeyPtr>::Hash, SteamBot::SmartDerefStuff<AssetKeyPtr>::Equals> KeySet;

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Econ::GetAssetClassInfo)> GetAssetClassInfoInfo;

/************************************************************************/

namespace
{
    class AssetData
    {
    public:
        typedef std::vector<AssetKeyPtr> KeyList;
        typedef std::vector<std::pair<uint32_t, KeyList>> MissingKeys;

    private:
        mutable boost::fibers::mutex mutex;
        KeySet data;

    private:
        void storeReceivedData(std::shared_ptr<GetAssetClassInfoInfo::ResultType>);
        void requestData(const MissingKeys&);
        MissingKeys getMissingKeys(const KeySet&) const;

    private:
        AssetData() =default;
        ~AssetData() =default;

    public:
        static AssetData& get()
        {
            static AssetData& instance=*new AssetData;
            return instance;
        }

    public:
        void update(const KeySet&);
        AssetInfoPtr query(const AssetKeyPtr&) const;
    };
};

/************************************************************************/

AssetInfo::~AssetInfo() =default;

/************************************************************************/

boost::json::value AssetInfo::toJson() const
{
    boost::json::object object;
    object["asset-key"]=AssetKey::toJson();
    SteamBot::enumToJson(object, "itemType", itemType);
    if (!name.empty()) object["name"]=name;
    if (!type.empty()) object["type"]=type;
    if (marketFeeApp!=SteamBot::AppID::None) object["marketFeeApp"]=static_cast<std::underlying_type_t<SteamBot::AppID>>(marketFeeApp);
    return object;
}

/************************************************************************/

AssetData::MissingKeys AssetData::getMissingKeys(const KeySet& items) const
{
    AssetData::MissingKeys missing;
    for (const auto& key : items)
    {
        assert(key->appId!=0 && key->classId!=0);
        if (!data.contains(key))
        {
            auto iterator=missing.begin();
            while (true)
            {
                if (iterator==missing.end())
                {
                    auto& chunk=missing.emplace_back();
                    chunk.first=key->appId;
                    chunk.second.push_back(key);
                    break;
                }

                if (iterator->first==key->appId)
                {
                    iterator->second.push_back(key);
                    break;
                }

                ++iterator;
            }
        }
    }
    return missing;
}

/************************************************************************/

static AssetInfo::ItemType checkItemType_TradingCard(const boost::json::value& json, const AssetInfo& assetInfo)
{
    for (const auto& ownerAction : json.at("owner_actions").as_array())
    {
        if (auto name=SteamBot::JSON::optString(ownerAction, "name"))
        {
            if (*name==std::string_view("#Profile_ViewBadgeProgress"))
            {
                if (auto link=SteamBot::JSON::optString(ownerAction, "link"))
                {
                    std::string_view string(*link);
                    if (SteamBot::AssetKey::parseString(string, "https://steamcommunity.com/my/gamecards/"))
                    {
                        uint32_t number;
                        if (SteamBot::AssetKey::parseNumberSlash(string, number))
                        {
                            assert(static_cast<SteamBot::AppID>(number)==assetInfo.marketFeeApp);
                            assert(string.size()==0);
                            return AssetInfo::ItemType::TradingCard;
                        }
                    }
                }
            }
        }
    }
    return AssetInfo::ItemType::Unknown;
}

/************************************************************************/

static AssetInfo::ItemType checkItemType_Emoticon(const boost::json::value& json, const AssetInfo&)
{
    for (const auto& description : json.at("descriptions").as_array())
    {
        if (auto type=SteamBot::JSON::optString(description, "type"); type!=nullptr && *type=="html")
        {
            if (auto value=SteamBot::JSON::optString(description, "value"))
            {
                if (value->find("class=\"emoticon\"")!=std::string::npos)
                {
                    return AssetInfo::ItemType::Emoticon;
                }
            }
        }
    }
    return AssetInfo::ItemType::Unknown;
}

/************************************************************************/

static AssetInfo::ItemType checkItemType_Gems(const boost::json::value&, const AssetInfo& assetInfo)
{
    return (assetInfo.type=="Steam Gems") ? AssetInfo::ItemType::Gems : AssetInfo::ItemType::Unknown;
}

/************************************************************************/
/*
 * This takes a JSON corresponding to a CEconItem_Description.
 */

AssetInfo::AssetInfo(const boost::json::value& json)
{
    if (!AssetKey::init(json)) throw false;

    SteamBot::JSON::optString(json, "name", name);
    SteamBot::JSON::optString(json, "type", type);
    SteamBot::JSON::optNumber(json, "market_fee_app", marketFeeApp);

    {
        typedef AssetInfo::ItemType(*ItemTypeCheck)(const boost::json::value&, const AssetInfo&);
        static const ItemTypeCheck itemTypeChecks[]={
            &checkItemType_TradingCard,
            &checkItemType_Emoticon,
            &checkItemType_Gems
        };

        for (auto checkFunction : itemTypeChecks)
        {
            auto detected=checkFunction(json, *this);
            if (detected!=AssetInfo::ItemType::Unknown)
            {
                assert(itemType==AssetInfo::ItemType::Unknown);
                itemType=detected;
            }
        }
    }
}

/************************************************************************/
/*
 * Note: this is a bit annoying, but the Inventory web query gives
 * us the same kind of data in JSON. So, to simplify the code,
 * I'm now converting the protobuf to JSON, and the rest of the
 * code will process the JSON (effectively converting it back into
 * binary...).
 *
 * Yes, this is inefficient... but I feel it's easier that way,
 * instead of trying to convert the JSON into the protobuf message.
 */

void AssetData::storeReceivedData(std::shared_ptr<GetAssetClassInfoInfo::ResultType> response)
{
    for (int i=0; i<response->descriptions_size(); i++)
    {
        auto json=SteamBot::toJson(dynamic_cast<const google::protobuf::Message&>(response->descriptions(i)));
        auto info=std::make_shared<AssetInfo>(json);
        BOOST_LOG_TRIVIAL(debug) << info->toJson();

        auto success=data.insert(std::move(info)).second;
        assert(success);
    }
}

/************************************************************************/

void AssetData::requestData(const MissingKeys& missing)
{
    for (const auto& chunk : missing)
    {
        std::shared_ptr<GetAssetClassInfoInfo::ResultType> response;
        {
            GetAssetClassInfoInfo::RequestType request;
            request.set_language("english");
            request.set_appid(chunk.first);
            for (const auto& key : chunk.second)
            {
                auto& item=*(request.add_classes());
                item.set_classid(key->classId);
                if (key->instanceId!=0)
                {
                    item.set_instanceid(key->instanceId);
                }
            }
            response=SteamBot::Modules::UnifiedMessageClient::execute<GetAssetClassInfoInfo::ResultType>("Econ.GetAssetClassInfo#1", std::move(request));
            storeReceivedData(std::move(response));
        }
    }
}

/************************************************************************/

void AssetData::update(const KeySet& items)

{
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto missing=getMissingKeys(items);
    requestData(missing);
}

/************************************************************************/
/*
 * This does NOT fetch new data
 */

AssetInfoPtr AssetData::query(const AssetKeyPtr& key) const
{
    AssetInfoPtr result;
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto iterator=data.find(key);
    if (iterator!=data.end())
    {
        result=std::dynamic_pointer_cast<AssetInfo>(*iterator);
    }

    if (result)
    {
        assert(static_cast<const SteamBot::AssetKey&>(*result)==*key);
    }

    return result;
}

/************************************************************************/

namespace
{
    class AssetDataModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<IncomingTradeOffers> incomingTradeOffers;

    public:
        void handle(std::shared_ptr<const IncomingTradeOffers>);

    public:
        AssetDataModule() =default;
        virtual ~AssetDataModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    AssetDataModule::Init<AssetDataModule> init;
}

/************************************************************************/

MyIncomingTradeOffers::~IncomingTradeOffers() =default;

MyIncomingTradeOffers::TradeOfferAssets::~TradeOfferAssets() =default;

/************************************************************************/

MyIncomingTradeOffers::TradeOfferAssets::TradeOfferAssets(decltype(offer)& offer_)
    : offer(offer_)
{
    const auto& assetData=::AssetData::get();

    myItems.reserve(offer.myItems.size());
    for (auto& item : offer.myItems)
    {
        myItems.emplace_back(assetData.query(item));
    }

    theirItems.reserve(offer.theirItems.size());
    for (auto& item : offer.theirItems)
    {
        theirItems.emplace_back(assetData.query(item));
    }
}

/************************************************************************/
/*
 * This is for human reading; it mixes information from the
 * TradeOffers and the AssetData stuff to make it look unified.
 */

boost::json::value MyIncomingTradeOffers::TradeOfferAssets::toJson() const
{
    struct Foo
    {
        static void merge(boost::json::object& json, std::string_view key, const std::vector<std::shared_ptr<const AssetInfo>>& assets)
        {
            auto& jsonArray=json.at(key).as_array();
            assert(jsonArray.size()==assets.size());
            for (size_t i=0; i<assets.size(); i++)
            {
                auto& arrayObject=jsonArray[i].as_object();
                auto myObject=assets[i]->toJson();
                for (auto& item : myObject.as_object())
                {
                    auto iterator=arrayObject.find(item.key());
                    if (iterator==arrayObject.end())
                    {
                        arrayObject.insert(item);
                    }
                    else
                    {
                        assert(iterator->value()==item.value());
                    }
                }
            }
        }
    };

    auto json=offer.toJson();
    Foo::merge(json.as_object(), "myItems", myItems);
    Foo::merge(json.as_object(), "theirItems", theirItems);
    return json;
};

/************************************************************************/

boost::json::value MyIncomingTradeOffers::toJson() const
{
    boost::json::array json;
    for (const auto& asset : assets)
    {
        json.emplace_back(asset.toJson());
    }
    return json;
}

/************************************************************************/

MyIncomingTradeOffers::IncomingTradeOffers(std::shared_ptr<const ::IncomingTradeOffers>&& offers_)
    : offers(std::move(offers_))
{
    assets.reserve(offers->offers.size());
    for (const auto& offer : offers->offers)
    {
        assets.emplace_back(*offer);
    }
}

/************************************************************************/

void AssetDataModule::handle(std::shared_ptr<const IncomingTradeOffers> message)
{
    {
        KeySet keys;
        for (const auto& offer: message->offers)
        {
            keys.insert(offer->myItems.begin(), offer->myItems.end());
            keys.insert(offer->theirItems.begin(), offer->theirItems.end());
        }
        ::AssetData::get().update(keys);
    }

    auto result=std::make_shared<MyIncomingTradeOffers>(std::move(message));
    SteamBot::UI::OutputText() << "detected incoming trade offers: " << result->toJson();
    getClient().messageboard.send(std::move(result));
}

/************************************************************************/

void AssetDataModule::init(SteamBot::Client& client)
{
    incomingTradeOffers=client.messageboard.createWaiter<IncomingTradeOffers>(*waiter);
}

/************************************************************************/

void AssetDataModule::run(SteamBot::Client&)
{
    while (true)
    {
        waiter->wait();
        incomingTradeOffers->handle(this);
    }
}

/************************************************************************/

void SteamBot::Modules::AssetData::use()
{
}
