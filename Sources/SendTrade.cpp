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

#include "SendTrade.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/TradeToken.hpp"
#include "Modules/Inventory.hpp"
#include "Web/URLEncode.hpp"
#include "Helpers/JSON.hpp"
#include "SteamID.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::SendTrade SendTrade;

/************************************************************************/

SendTrade::SendTrade() =default;
SendTrade::~SendTrade() =default;

SendTrade::Item::Item() =default;
SendTrade::Item::~Item() =default;

/************************************************************************/

namespace
{
    class ErrorException { };
}

/************************************************************************/

SendTrade::Item::Item(const SteamBot::Modules::Inventory::InventoryItem& item)
{
    appId=item.appId;
    contextId=item.contextId;
    assetId=item.assetId;
    amount=item.amount;
}

/************************************************************************/
/*
 * Make the form data string, except for
 *   - "sessionid"
 *   - "json_tradeoffer"
 *
 * Also returns the SteamUD.
 */

static std::string makeBasicFormData(const SendTrade& sendTrade, SteamBot::SteamID& steamId)
{
    std::string body;

    auto& dataFile=SteamBot::DataFile::get(sendTrade.partner->accountName, SteamBot::DataFile::FileType::Account);

    {
        std::string tradeToken=SteamBot::Modules::TradeToken::get(dataFile);
        if (tradeToken.empty())
        {
            BOOST_LOG_TRIVIAL(info) << "no trade token for " << sendTrade.partner->accountName;
            throw ErrorException();
        }

        boost::json::object object;
        object["trade_offer_access_token"]=std::move(tradeToken);
        SteamBot::Web::formUrlencode(body, "trade_offer_create_params", boost::json::serialize(object));
    }

    {
        dataFile.examine([&steamId, &sendTrade](const boost::json::value& json) {
            if (auto value=SteamBot::JSON::getItem(json, "Info", "SteamID"))
            {
                steamId.setValue(value->to_number<uint64_t>());
            }
            else
            {
                BOOST_LOG_TRIVIAL(info) << "no SteamID for " << sendTrade.partner->accountName;
                throw ErrorException();
            }
        });

        SteamBot::Web::formUrlencode(body, "partner", steamId.getValue());
    }

    SteamBot::Web::formUrlencode(body, "serverid", 1);		// ??? ASF hardcodes this, no explanation
    SteamBot::Web::formUrlencode(body, "tradeoffermessage", "Sent by Christians-Steam-Framework");

    return body;
}

/************************************************************************/

static boost::json::object makeTradeOfferData(const SendTrade& sendTrade)
{
    class MakeJSON : public boost::json::object
    {
    private:
        boost::json::object makeItems(const std::vector<SendTrade::Item>& items)
        {
            boost::json::object json;
            {
                boost::json::array array;
                for (const auto item : items)
                {
                    boost::json::object object;
                    object["appid"]=std::to_string(toInteger(item.appId));
                    object["contextid"]=std::to_string(toInteger(item.contextId));
                    object["assetid"]=std::to_string(toInteger(item.assetId));
                    object["amount"]=(item.amount==0 ? 1 : item.amount);
                    array.emplace_back(std::move(object));
                }
                json["assets"]=std::move(array);
            }
            json["currency"]=boost::json::array();
            json["ready"]=false;
            return json;
        }

    public:
        MakeJSON(const SendTrade& sendTrade)
        {
            (*this)["newversion"]=true;
            (*this)["version"]=2;
            (*this)["me"]=makeItems(sendTrade.myItems);
            (*this)["them"]=makeItems(sendTrade.theirItems);
        }
    };

    return MakeJSON(sendTrade);
}

/************************************************************************/

static std::shared_ptr<Request> makeTradeOfferRequest(const SendTrade& sendTrade)
{
    static const boost::urls::url_view baseUrl{"https://steamcommunity.com/tradeoffer/new"};

    class Params
    {
    public:
        std::string body;
        std::string referer;
        boost::urls::url url;

    public:
        Params(const SendTrade& sendTrade)
        {
            SteamBot::SteamID steamId;
            body=makeBasicFormData(sendTrade, steamId);
            {
                auto tradeOfferData=makeTradeOfferData(sendTrade);
                SteamBot::Web::formUrlencode(body, "json_tradeoffer", boost::json::serialize(tradeOfferData));
            }

            url=baseUrl;
            {
                url.params().set("partner", std::to_string(toInteger(steamId.getAccountId())));
                referer=url.buffer();
                url.params().clear();
            }

            url.segments().push_back("send");
        }
    };

    std::shared_ptr<Request> request;

    auto params=std::make_shared<Params>(sendTrade);
    request=std::make_shared<Request>();
    request->queryMaker=[params=std::move(params)]() {
        {
            auto cookies=SteamBot::Client::getClient().whiteboard.has<SteamBot::Modules::WebSession::Whiteboard::Cookies>();
            assert(cookies!=nullptr);
            SteamBot::Web::formUrlencode(params->body, "sessionid", cookies->sessionid);
        }

        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::post, std::move(params->url));
        query->request.set(boost::beast::http::field::referer, params->referer);
        query->request.body()=std::move(params->body);
        query->request.content_length(query->request.body().size());
        query->request.base().set("Content-Type", "application/x-www-form-urlencoded");
        return query;
    };

    return request;
}

/************************************************************************/

bool SendTrade::send() const
{
    try
    {
        auto request=makeTradeOfferRequest(*this);
        auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        auto string=SteamBot::HTTPClient::parseString(*(response->query));
        return true;
    }
    catch(const ErrorException&)
    {
        return false;
    }
}
