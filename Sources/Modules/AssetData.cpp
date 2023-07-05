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
#include "AssetKey.hpp"
#include "DerefStuff.hpp"
#include "Modules/AssetData.hpp"
#include "Modules/TradeOffers.hpp"
#include "Modules/UnifiedMessageClient.hpp"

#include <unordered_set>

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::TradeOffers::Messageboard::IncomingTradeOffers IncomingTradeOffers;

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
        boost::fibers::mutex mutex;
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
        std::vector<AssetInfoPtr> query(const KeySet&);
    };
};

/************************************************************************/

AssetInfo::~AssetInfo() =default;

/************************************************************************/

boost::json::value AssetInfo::toJson() const
{
    boost::json::object object;
    object["key"]=AssetKey::toJson();
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

AssetInfo::AssetInfo(const ::CEconItem_Description& description)
{
    assert(description.has_appid());
    appId=description.appid();

    assert(description.has_classid());
    classId=description.classid();

    if (description.has_instanceid())
    {
        instanceId=description.instanceid();
    }

    if (description.has_name())
    {
        name=description.name();
    }
    if (description.has_type())
    {
        type=description.type();

        if (type=="Steam Gems")
        {
            assert(itemType==AssetInfo::ItemType::Unknown);
            itemType=AssetInfo::ItemType::Gems;
        }
    }

    if (description.has_market_fee_app())
    {
        marketFeeApp=static_cast<SteamBot::AppID>(description.market_fee_app());
    }

    for (int j=0; j<description.owner_actions_size(); j++)
    {
        const auto& ownerAction=description.owner_actions(j);
        if (ownerAction.has_name())
        {
            if (ownerAction.name()=="#Profile_ViewBadgeProgress")
            {
                assert(itemType==AssetInfo::ItemType::Unknown);
                itemType=AssetInfo::ItemType::TradingCard;

                if (ownerAction.has_link())
                {
                    std::string_view string=ownerAction.link();
                    if (SteamBot::AssetKey::parseString(string, "https://steamcommunity.com/my/gamecards/"))
                    {
                        uint32_t number;
                        if (SteamBot::AssetKey::parseNumberSlash(string, number))
                        {
                            assert(static_cast<SteamBot::AppID>(number)==marketFeeApp);
                        }
                    }
                    assert(string.size()==0);
                }
            }
        }
    }
}

/************************************************************************/

void AssetData::storeReceivedData(std::shared_ptr<GetAssetClassInfoInfo::ResultType> response)
{
    for (int i=0; i<response->descriptions_size(); i++)
    {
        auto info=std::make_shared<AssetInfo>(response->descriptions(i));
        BOOST_LOG_TRIVIAL(debug) << info->toJson();
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

std::vector<AssetInfoPtr> AssetData::query(const KeySet& items)

{
    std::vector<AssetInfoPtr> result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto missing=getMissingKeys(items);
        requestData(missing);

        for (const auto& key: items)
        {
            auto iterator=data.find(key);
            if (iterator!=data.end())
            {
                result.emplace_back(std::dynamic_pointer_cast<AssetInfo>(*iterator));
            }
            else
            {
                result.emplace_back();
            }
        }
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

void AssetDataModule::handle(std::shared_ptr<const IncomingTradeOffers> message)
{
    KeySet keys;
    for (const auto& offer: message->offers)
    {
        keys.insert(offer->myItems.begin(), offer->myItems.end());
        keys.insert(offer->theirItems.begin(), offer->theirItems.end());
    }
    AssetData::get().query(keys);
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
