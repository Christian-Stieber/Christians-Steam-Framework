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
 * Inspired by ASF's SendTradeOffer()
 */

/************************************************************************/

#include "SendTrade.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/TradeToken.hpp"
#include "Modules/Inventory.hpp"
#include "Web/URLEncode.hpp"
#include "Helpers/JSON.hpp"
#include "SteamID.hpp"
#include "UI/UI.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

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

SendTrade::Item::Item(const SteamBot::Inventory::Item& item)
    : ItemKey(item)
{
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
                for (const auto& item : items)
                {
                    boost::json::object myObject;
                    myObject["appid"]=std::to_string(toInteger(item.appId));
                    myObject["contextid"]=std::to_string(toInteger(item.contextId));
                    myObject["assetid"]=std::to_string(toInteger(item.assetId));
                    myObject["amount"]=(item.amount==0 ? 1 : item.amount);
                    array.emplace_back(std::move(myObject));
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

namespace
{
    static const boost::urls::url_view baseUrl{"https://steamcommunity.com/tradeoffer/new"};

    class Params : public SteamBot::Modules::WebSession::PostWithSession
    {
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
}

/************************************************************************/

static std::shared_ptr<const Response> makeTradeOfferRequest(const SendTrade& sendTrade)
{
    return Params(sendTrade).execute();
}

/************************************************************************/

bool SendTrade::send() const
{
    try
    {
        auto response=makeTradeOfferRequest(*this);
        if (response->query->response.result()==boost::beast::http::status::ok)
        {
            auto json=SteamBot::HTTPClient::parseJson(*(response->query));

            SteamBot::UI::OutputText output;
            output << "created trade with id " << toInteger(SteamBot::JSON::toNumber<SteamBot::TradeOfferID>(json.at("tradeofferid")));
            if (SteamBot::JSON::optBoolDefault(json, "needs_mobile_confirmation"))
            {
                output << "; needs mobile confirmation";
            }
            else if (SteamBot::JSON::optBoolDefault(json, "needs_email_confirmation"))
            {
                output << "; needs email confirmation @" << static_cast<std::string_view>(json.at("email_domain").as_string());
            }
            return true;
        }
    }
    catch(const ErrorException&)
    {
    }
    return false;
}
