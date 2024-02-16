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

#include "Connection/Serialize.hpp"
#include "Client/Client.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/SaleSticker.hpp"
#include "Web/URLEncode.hpp"
#include "UI/UI.hpp"
#include "Helpers/Time.hpp"
#include "Helpers/JSON.hpp"
#include "Random.hpp"
#include "Base64.hpp"
#include "EnumString.hpp"
#include "Exceptions.hpp"

#include <boost/exception/diagnostic_information.hpp>

#include "Helpers/ProtoPuf.hpp"

/************************************************************************/
/*
 * Adapted from webui/service_saleitemrewards.proto
 *
 * Note: I need the namespace just for emacs to get the formatting right
 */

namespace
{
    using LoyaltyRewardDefinition_BadgeData = pp::message<
        pp::int32_field<"level", 1>,
        pp::string_field<"image", 2>
        >;

    using LoyaltyRewardDefinition_CommunityItemData = pp::message<
        pp::string_field<"item_name", 1>,
        pp::string_field<"item_title", 2>,
        pp::string_field<"item_description", 3>,
        pp::string_field<"item_image_small", 4>,
        pp::string_field<"item_image_large", 5>,
        pp::string_field<"item_movie_webm", 6>,
        pp::string_field<"item_movie_mp4", 7>,
        pp::bool_field<"animated", 8>,
        pp::message_field<"badge_data", 9, LoyaltyRewardDefinition_BadgeData, pp::repeated>,
        pp::string_field<"item_movie_webm_small", 10>,
        pp::string_field<"item_movie_mp4_small", 11>,
        pp::string_field<"profile_theme_id", 12>
        >;

    using LoyaltyRewardDefinition = pp::message<
        pp::uint32_field<"appid", 1>,
        pp::uint32_field<"defid", 2>,
        pp::int32_field<"type", 3>,
        pp::int32_field<"community_item_class", 4>,
        pp::uint32_field<"community_item_type", 5>,
        pp::int64_field<"point_cost", 6>,
        pp::uint32_field<"timestamp_created", 7>,
        pp::uint32_field<"timestamp_updated", 8>,
        pp::uint32_field<"timestamp_available", 9>,
        pp::int64_field<"quantity", 10>,
        pp::string_field<"internal_description", 11>,
        pp::bool_field<"active", 12>,
        pp::message_field<"community_item_data", 13, LoyaltyRewardDefinition_CommunityItemData>,
        pp::uint32_field<"timestamp_available_end", 14>,
        pp::uint32_field<"bundle_defids", 15, pp::repeated>,
        pp::uint32_field<"usable_duration", 16>,
        pp::uint32_field<"bundle_discount", 17>
        >;

    using CSaleItemRewards_ClaimItem_Response = pp::message<
        pp::uint64_field<"communityitemid", 1>,
        pp::uint32_field<"next_claim_time", 2>,
        pp::message_field<"reward_item", 3, LoyaltyRewardDefinition>
        >;

    using CSaleItemRewards_ClaimItem_Request = pp::message<
        pp::string_field<"language", 1>
        >;
}

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::SaleSticker::Status Status;

typedef SteamBot::HTTPClient::Query Query;

/************************************************************************/

namespace
{
    class MyClaim : public Status
    {
    private:
        std::string token;

    private:
        void initItem(const boost::json::value&);

        void claim();
        bool canClaim();

    public:
        MyClaim();
    };
}

/************************************************************************/

Status::Status()
    : when(std::chrono::system_clock::now())
{
}

/************************************************************************/

Status::~Status() =default;

Status::Item::Item() =default;
Status::Item::~Item() =default;

/************************************************************************/

bool MyClaim::canClaim()
{
    assert(nextClaimTime.time_since_epoch().count()==0);
    assert(result==Status::ClaimResult::Invalid);

    std::unique_ptr<Query> query;
    {
        boost::urls::url url("https://api.steampowered.com/ISaleItemRewardsService/CanClaimItem/v1");
        url.params().set("access_token", token);

        query=std::make_unique<Query>(boost::beast::http::verb::get, std::move(url));
        query=SteamBot::HTTPClient::perform(std::move(query));
    }

    if (query->response.result()==boost::beast::http::status::ok)
    {
        auto json=SteamBot::HTTPClient::parseJson(*query);
        BOOST_LOG_TRIVIAL(debug) << json;
        // {"response":{"can_claim":true,"next_claim_time":1688230800}}

        auto& response=json.at("response");

        {
            std::time_t next_claim_time;
            if (SteamBot::JSON::optNumber(response, "next_claim_time", next_claim_time))
            {
                nextClaimTime=std::chrono::system_clock::from_time_t(next_claim_time);
            }
        }

        {
            bool canClaim=false;
            if (SteamBot::JSON::optBool(response, "can_claim", canClaim))
            {
                if (canClaim)
                {
                    return true;
                }
                else
                {
                    BOOST_LOG_TRIVIAL(debug) << "SaleSticker: already claimed";
                    result=Status::ClaimResult::AlreadyClaimed;
                }
            }
            else
            {
                BOOST_LOG_TRIVIAL(debug) << "SaleSticker: no sale";
                result=Status::ClaimResult::NoSale;
            }
        }
    }
    else
    {
        result=Status::ClaimResult::Error;
    }
    return false;
}

/************************************************************************/

void MyClaim::initItem(const boost::json::value& json)
{
    try
    {
        const auto& reward=json.at("reward_item");
        item.type=SteamBot::JSON::toNumber<decltype(item.type)>(reward.at("community_item_class"));

        const auto& data=reward.at("community_item_data");
        SteamBot::JSON::optString(data, "item_name", item.name);
        SteamBot::JSON::optString(data, "item_title", item.title);
        SteamBot::JSON::optString(data, "item_description", item.description);
    }
    catch(...)
    {
    }
}

/************************************************************************/

void MyClaim::claim()
{
    assert(result==Status::ClaimResult::Invalid);

    std::unique_ptr<Query> query;
    {
        boost::urls::url url("https://api.steampowered.com/ISaleItemRewardsService/ClaimItem/v1");
        url.params().set("access_token", token);

        auto bytes=SteamBot::ProtoPuf::encode(CSaleItemRewards_ClaimItem_Request("english"));

        std::string body;
        SteamBot::Web::formUrlencode(body, "input_protobuf_encoded", SteamBot::Base64::encode(bytes));

        query=std::make_unique<Query>(boost::beast::http::verb::post, std::move(url));
        query->request.body()=std::move(body);
        query->request.content_length(query->request.body().size());
        query->request.base().set("Content-Type", "application/x-www-form-urlencoded");

        query=SteamBot::HTTPClient::perform(std::move(query));
    }

    if (query->response.result()==boost::beast::http::status::ok)
    {
        boost::json::value json;
        {
            auto body=SteamBot::HTTPClient::parseString(*query);
            std::span<std::byte> bodyBytes(static_cast<std::byte*>(static_cast<void*>(body.data())), body.size());
            auto response=SteamBot::ProtoPuf::decode<CSaleItemRewards_ClaimItem_Response>(bodyBytes);
            json=SteamBot::ProtoPuf::toJson(response);
        }
        BOOST_LOG_TRIVIAL(debug) << json;
        initItem(json);

        result=Status::ClaimResult::Ok;
    }
    else
    {
        result=Status::ClaimResult::Error;
    }
}

/************************************************************************/

MyClaim::MyClaim()
{
    try
    {
        token=SteamBot::Modules::WebSession::getAccessToken();
        if (canClaim())
        {
            claim();
        }
    }
    catch(const SteamBot::OperationCancelledException&)
    {
        throw;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "SaleSticker: exception " << boost::current_exception_diagnostic_information();
        result=Status::ClaimResult::Error;
    }

    assert(result!=ClaimResult::Invalid);

    BOOST_LOG_TRIVIAL(debug) << "SaleSticker status: " << toJson();
}

/************************************************************************/

boost::json::value Status::Item::toJson() const
{
    boost::json::object json;
    if (type!=SteamBot::CommunityItemClass::Invalid)
    {
        SteamBot::enumToJson(json, "type", type);
        if (!name.empty()) json["name"]=name;
        if (!title.empty()) json["title"]=title;
        if (!description.empty()) json["description"]=description;
    }
    return json;
}

/************************************************************************/

boost::json::value Status::toJson() const
{
    boost::json::object json;
    SteamBot::enumToJson(json, "result", result);
    json["when"]=SteamBot::Time::toString(when);
    if (nextClaimTime.time_since_epoch().count()!=0) json["nextClaimTime"]=SteamBot::Time::toString(nextClaimTime);
    {
        auto object=item.toJson();
        if (!object.as_object().empty())
        {
            json["item"]=std::move(object);
        }
    }
    return json;
}

/************************************************************************/

const Status& SteamBot::SaleSticker::claim()
{
    static thread_local boost::fibers::mutex mutex;
    std::lock_guard<decltype(mutex)> lock(mutex);

    auto& whiteboard=SteamBot::Client::getClient().whiteboard;
    whiteboard.set<Status>(MyClaim());

    auto result=whiteboard.has<Status>();
    assert(result!=nullptr);
    return *result;
}
