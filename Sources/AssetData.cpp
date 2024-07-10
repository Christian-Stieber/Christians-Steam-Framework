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

#include "AssetData.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Helpers/JSON.hpp"
#include "Helpers/ProtoBuf.hpp"
#include "Helpers/ParseNumber.hpp"
#include "UI/UI.hpp"

#include <unordered_set>

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::AssetData::AssetInfo AssetInfo;
typedef std::shared_ptr<const AssetInfo> AssetInfoPtr;

typedef SteamBot::AssetData::KeyPtr KeyPtr;
typedef SteamBot::AssetData::KeySet KeySet;

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Econ::GetAssetClassInfo)> GetAssetClassInfoInfo;

/************************************************************************/

namespace
{
    class AssetData
    {
    public:
        typedef std::vector<KeyPtr> KeyList;
        typedef std::vector<std::pair<SteamBot::AppID, KeyList>> MissingKeys;

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
        AssetInfoPtr store(const boost::json::value&);
        void fetch(const KeySet&);
        AssetInfoPtr query(const KeyPtr&) const;
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
    if (marketFeeApp!=SteamBot::AppID::None) object["marketFeeApp"]=toInteger(marketFeeApp);
    return object;
}

/************************************************************************/

AssetData::MissingKeys AssetData::getMissingKeys(const KeySet& items) const
{
    AssetData::MissingKeys missing;
    for (const auto& key : items)
    {
        assert(key->appId!=SteamBot::AppID::None && key->classId!=SteamBot::ClassID::None);
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
    if (auto ownerActions=json.as_object().if_contains("owner_actions"))
    {
        for (const auto& ownerAction : ownerActions->as_array())
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
                            auto appId=SteamBot::AppID::None;
                            if (SteamBot::parseNumberSlash(string, appId))
                            {
                                // ToDo: there could be a "?border=1" following (and maybe other stuff?)
                                assert(appId==assetInfo.marketFeeApp);
                                assert(string.size()==0 || string[0]=='?');
                                return AssetInfo::ItemType::TradingCard;
                            }
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
    for (const auto& myDescription : json.at("descriptions").as_array())
    {
        if (auto type=SteamBot::JSON::optString(myDescription, "type"); type!=nullptr && *type=="html")
        {
            if (auto value=SteamBot::JSON::optString(myDescription, "value"))
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
    SteamBot::JSON::optBool(json, "tradable", isTradable);

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

AssetInfoPtr AssetData::store(const boost::json::value& json)
{
    try
    {
        auto info=std::make_shared<AssetInfo>(json);
        BOOST_LOG_TRIVIAL(debug) << info->toJson();

        return data.insert(info).second ? info : nullptr;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "unable to process " << json;
        return nullptr;
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
        auto info=store(json);
        assert(info);
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
            assert(chunk.first!=SteamBot::AppID::None);
            const auto appId=toInteger(chunk.first);
            assert(appId>=0);
            request.set_appid(static_cast<uint32_t>(appId));
            for (const auto& key : chunk.second)
            {
                auto& item=*(request.add_classes());
                assert(key->classId!=SteamBot::ClassID::None);
                item.set_classid(toInteger(key->classId));
                if (key->instanceId!=SteamBot::InstanceID::None)
                {
                    item.set_instanceid(toInteger(key->instanceId));
                }
            }
            response=SteamBot::Modules::UnifiedMessageClient::execute<GetAssetClassInfoInfo::ResultType>("Econ.GetAssetClassInfo#1", std::move(request));
            storeReceivedData(std::move(response));
        }
    }
}

/************************************************************************/

void AssetData::fetch(const KeySet& items)

{
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto missing=getMissingKeys(items);
    requestData(missing);
}

/************************************************************************/
/*
 * This does NOT fetch new data
 */

AssetInfoPtr AssetData::query(const KeyPtr& key) const
{
    AssetInfoPtr result;
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto iterator=data.find(key);
    if (iterator!=data.end())
    {
        result=std::dynamic_pointer_cast<const AssetInfo>(*iterator);
    }

    if (result)
    {
        assert(static_cast<const SteamBot::AssetKey&>(*result)==*key);
    }

    return result;
}

/************************************************************************/

void SteamBot::AssetData::fetch(const KeySet& keys)
{
    ::AssetData::get().fetch(keys);
}

/************************************************************************/

AssetInfoPtr SteamBot::AssetData::query(KeyPtr key)
{
    return ::AssetData::get().query(key);
}

/************************************************************************/

AssetInfoPtr SteamBot::AssetData::store(const boost::json::value& json)
{
    return ::AssetData::get().store(json);
}
