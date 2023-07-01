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
#include "Modules/WebSession.hpp"
#include "Modules/SaleSticker.hpp"
#include "Web/URLEncode.hpp"
#include "Connection/Serialize.hpp"
#include "UI/UI.hpp"
#include "Helpers/Time.hpp"
#include "Helpers/JSON.hpp"
#include "Steam/CommunityItemClass.hpp"
#include "Random.hpp"
#include "Base64.hpp"
#include "EnumString.hpp"

#include <regex>

#include <boost/url/url_view.hpp>

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

typedef SteamBot::Modules::SaleSticker::Messageboard::ClaimSaleSticker ClaimSaleSticker;

typedef SteamBot::HTTPClient::Query Query;

/************************************************************************/

ClaimSaleSticker::ClaimSaleSticker(bool force_)
    : force(force_)
{
}

ClaimSaleSticker::~ClaimSaleSticker() =default;

/************************************************************************/

namespace
{
    class SaleStickerModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<ClaimSaleSticker> claimSaleStickerWaiter;

    private:
        static void claimItem(std::string_view);
        static bool canClaimItem(std::string_view);
        static std::string getIoken();

    public:
        SaleStickerModule()
            : claimSaleStickerWaiter(getClient().messageboard.createWaiter<ClaimSaleSticker>(*waiter))
        {
        }

        virtual ~SaleStickerModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    SaleStickerModule::Init<SaleStickerModule> init;
}

/************************************************************************/
/*
 * Let's pretend to be a user -- I'll select one randomly :-)
 * I should probably try to get this list from the store, though...
 */

static const std::array<boost::urls::url_view, 20> categoryUrls={{
        boost::urls::url_view("https://store.steampowered.com/category/casual"),
        boost::urls::url_view("https://store.steampowered.com/category/fighting_martial_arts"),
        boost::urls::url_view("https://store.steampowered.com/greatondeck"),
        boost::urls::url_view("https://store.steampowered.com/category/simulation"),
        boost::urls::url_view("https://store.steampowered.com/category/visual_novel"),
        boost::urls::url_view("https://store.steampowered.com/genre/Free%20to%20Play"),
        boost::urls::url_view("https://store.steampowered.com/category/action"),
        boost::urls::url_view("https://store.steampowered.com/vr"),
        boost::urls::url_view("https://store.steampowered.com/category/survival"),
        boost::urls::url_view("https://store.steampowered.com/category/exploration_open_world"),
        boost::urls::url_view("https://store.steampowered.com/category/strategy"),
        boost::urls::url_view("https://store.steampowered.com/category/rpg"),
        boost::urls::url_view("https://store.steampowered.com/category/anime"),
        boost::urls::url_view("https://store.steampowered.com/category/rogue_like_rogue_lite"),
        boost::urls::url_view("https://store.steampowered.com/category/adventure"),
        boost::urls::url_view("https://store.steampowered.com/category/sports"),
        boost::urls::url_view("https://store.steampowered.com/category/story_rich"),
        boost::urls::url_view("https://store.steampowered.com/category/science_fiction"),
        boost::urls::url_view("https://store.steampowered.com/category/multiplayer_coop"),
        boost::urls::url_view("https://store.steampowered.com/category/horror")
    }};

/************************************************************************/
/*
 * Note: my HTML parser can't (currently?) parse these pages, but
 * I don't really need to.
 *
 * Note: apparently, when we access an account with a language that's
 * english, Steam will set a "Steam-Language" cookie and redirect to
 * the same page again. We don't currently support cookies like that
 * so this would lead to an endless redirection loop; we can work
 * around this by setting l=english on the URL.
 */

std::string SaleStickerModule::getIoken()
{
    std::string result;

    auto request=std::make_shared<Request>();
    request->queryMaker=[](){
        auto index=SteamBot::Random::generateRandomNumber()%categoryUrls.size();
        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, categoryUrls.at(index));
        query->url.params().set("l", "english");	// we should support cookies, but for now, this works
        return query;
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    static const std::regex regex(" data-loyalty_webapi_token=\"&quot;([a-f0-9]+)&quot;\"");
    std::smatch match;
    if (std::regex_search(html, match, regex))
    {
        assert(match.size()==2);
        result=match[1];
        BOOST_LOG_TRIVIAL(debug) << "SaleSticker: loyalty_webapi_token is \"" << result << "\"";
    }
    else
    {
        SteamBot::UI::OutputText() << "cannot find a token to claim a sticker";
    }

    return result;
}

/************************************************************************/

void SaleStickerModule::claimItem(std::string_view token)
{
    boost::urls::url url("https://api.steampowered.com/ISaleItemRewardsService/ClaimItem/v1");
    url.params().set("access_token", token);

    auto bytes=SteamBot::ProtoPuf::encode(CSaleItemRewards_ClaimItem_Request("english"));

    std::string body;
    SteamBot::Web::formUrlencode(body, "input_protobuf_encoded", SteamBot::Base64::encode(bytes));

    auto query=std::make_unique<Query>(boost::beast::http::verb::post, url);
    query->request.body()=std::move(body);
    query->request.content_length(query->request.body().size());
    query->request.base().set("Content-Type", "application/x-www-form-urlencoded");

    query=SteamBot::HTTPClient::perform(std::move(query));

    body=SteamBot::HTTPClient::parseString(*query);
    std::span<std::byte> bodyBytes(static_cast<std::byte*>(static_cast<void*>(body.data())), body.size());

    auto response=SteamBot::ProtoPuf::decode<CSaleItemRewards_ClaimItem_Response>(bodyBytes);
    BOOST_LOG_TRIVIAL(debug) << SteamBot::ProtoPuf::toJson(response);

    {
        using namespace pp;
        if (auto rewardItem=SteamBot::ProtoPuf::getItem(&response, "reward_item"_f))
        {
            SteamBot::UI::OutputText output;
            output << "claimed a community item";

            if (auto communityItemClass=SteamBot::ProtoPuf::getItem(rewardItem, "community_item_class"_f))
            {
                output << "; class " << SteamBot::enumToStringAlways(static_cast<SteamBot::CommunityItemClass>(*communityItemClass));
            }

            if (auto communityItemData=SteamBot::ProtoPuf::getItem(rewardItem, "community_item_data"_f))
            {
                if (auto itemName=SteamBot::ProtoPuf::getItem(communityItemData, "item_name"_f))
                {
                    output << "; name \"" << *itemName << "\"";
                }

                if (auto itemTitle=SteamBot::ProtoPuf::getItem(communityItemData, "item_title"_f))
                {
                    output << "; title \"" << *itemTitle << "\"";
                }

                if (auto itemDescription=SteamBot::ProtoPuf::getItem(communityItemData, "item_description"_f))
                {
                    output << "; description \"" << *itemDescription << "\"";
                }
            }
        }
    }
}

/************************************************************************/

bool SaleStickerModule::canClaimItem(std::string_view token)
{
    boost::urls::url url("https://api.steampowered.com/ISaleItemRewardsService/CanClaimItem/v1");
    url.params().set("access_token", token);

    auto query=std::make_unique<Query>(boost::beast::http::verb::get, url);

    query=SteamBot::HTTPClient::perform(std::move(query));

    auto json=SteamBot::HTTPClient::parseJson(*query);
    // {"response":{"can_claim":true,"next_claim_time":1688230800}}

    bool canClaim=false;
    if (auto can_claim=SteamBot::JSON::getItem(json, "response", "can_claim"))
    {
        if (auto can_claim_bool=can_claim->if_bool())
        {
            canClaim=*can_claim_bool;
        }
    }

    std::chrono::system_clock::time_point nextClaimTime;
    if (auto next_claim_time=SteamBot::JSON::getItem(json, "response", "next_claim_time"))
    {
        try
        {
            nextClaimTime=std::chrono::system_clock::from_time_t(next_claim_time->to_number<std::time_t>());
        }
        catch(const boost::json::system_error&)
        {
        }
    }

    SteamBot::UI::OutputText output;

    output << "a sale sticker ";
    output << (canClaim ? "can" : "can't");
    output << " be claimed";

    if (nextClaimTime.time_since_epoch().count()!=0)
    {
        output << "; next claim time is " << SteamBot::Time::toString(nextClaimTime, false);
    }

    return canClaim;
}

/************************************************************************/

void SaleStickerModule::run(SteamBot::Client& client)
{
    waitForLogin();

    while (true)
    {
        waiter->wait();

        if (auto message=claimSaleStickerWaiter->fetch())
        {
            auto token=getIoken();
            if (!token.empty())
            {
                if (canClaimItem(token))
                {
                    claimItem(token);
                }
            }

            while (claimSaleStickerWaiter->fetch())
                ;
        }
    }
}
