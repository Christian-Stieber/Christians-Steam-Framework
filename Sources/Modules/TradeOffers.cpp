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
#include "Modules/WebSession.hpp"
#include "Modules/TradeOffers.hpp"
#include "Helpers/URLs.hpp"
#include "Web/CookieJar.hpp"
#include "Helpers/HTML.hpp"
#include "Helpers/ParseNumber.hpp"
#include "UI/UI.hpp"

#include "HTMLParser/Parser.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::TradeOffers::TradeOffer TradeOffer;
typedef SteamBot::Modules::TradeOffers::IncomingTradeOffers IncomingTradeOffers;

/************************************************************************/

namespace
{
    class TradeOffersModule : public SteamBot::Client::Module
    {
    private:
        void parseIncomingTradeOffserPage(std::string_view) const;
        std::string getIncomingTradeOfferPage() const;

    public:
        TradeOffersModule() =default;
        virtual ~TradeOffersModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    TradeOffersModule::Init<TradeOffersModule> init;
}

/************************************************************************/

namespace
{
    class IncomingOffersParser : public HTMLParser::Parser
    {
    private:
        IncomingTradeOffers& result;

    private:
        class CurrentTradeOffer
        {
        public:
            const HTMLParser::Tree::Element& root;
            std::unique_ptr<TradeOffer> offer;

        public:
            CurrentTradeOffer(const HTMLParser::Tree::Element& element)
                : root(element),
                  offer(std::make_unique<TradeOffer>())
            {
            }
        };

        std::unique_ptr<CurrentTradeOffer> currentTradeOffer;

    public:
        IncomingOffersParser(std::string_view html, IncomingTradeOffers& result_)
            : Parser(html), result(result_)
        {
        }

        virtual ~IncomingOffersParser() =default;

    private:
        bool handleItemsBanner(const HTMLParser::Tree::Element&);
        bool handleTradePartner(const HTMLParser::Tree::Element&);
        bool handleTradeOffer_open(const HTMLParser::Tree::Element&);
        bool handleTradeOffer_close(const HTMLParser::Tree::Element&);

    private:
        virtual void startElement(const HTMLParser::Tree::Element& element) override
        {
            handleTradeOffer_open(element) || handleTradePartner(element);
        }

        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            handleTradeOffer_close(element) || handleItemsBanner(element);
        }
    };
}

/************************************************************************/

TradeOffer::TradeOffer() =default;
TradeOffer::~TradeOffer() =default;

IncomingTradeOffers::IncomingTradeOffers() =default;
IncomingTradeOffers::~IncomingTradeOffers() =default;

/************************************************************************/

boost::json::value TradeOffer::toJson() const
{
    boost::json::object json;
    json["tradeOfferId"]=tradeOfferId;
    json["partner"]=partner;
    return json;
}

/************************************************************************/

boost::json::value IncomingTradeOffers::toJson() const
{
    boost::json::array json;
    for (const auto& offer : offers)
    {
        json.emplace_back(offer->toJson());
    }
    return json;
}

/************************************************************************/

bool IncomingOffersParser::handleItemsBanner(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        // <div class="tradeoffer_items_banner in_escrow">
        if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items_banner"))
        {
            BOOST_LOG_TRIVIAL(info) << "ignoring trade offer " << currentTradeOffer->offer->tradeOfferId << ": " << SteamBot::HTML::getCleanText(element);
            currentTradeOffer.reset();
            return true;
        }
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleTradePartner(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        // <div class="tradeoffer_partner">
        //   <div class="playerAvatar offline" data-miniprofile="<steam32id>">
        if (element.name=="div" && SteamBot::HTML::checkClass(element, "playeravatar"))
        {
            if (auto parent=element.parent)
            {
                if (parent->name=="div" && SteamBot::HTML::checkClass(*parent, "tradeoffer_partner"))
                {
                    if (auto dataMiniprofile=element.getAttribute("data-miniprofile"))
                    {
                        if (!SteamBot::parseNumber(*dataMiniprofile, currentTradeOffer->offer->partner))
                        {
                            currentTradeOffer.reset();
                        }
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleTradeOffer_close(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer && &currentTradeOffer->root==&element)
    {
        if (currentTradeOffer->offer->tradeOfferId!=0 && currentTradeOffer->offer->partner!=0)
        {
            result.offers.push_back(std::move(currentTradeOffer->offer));
        }
        currentTradeOffer.reset();
        return true;
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleTradeOffer_open(const HTMLParser::Tree::Element& element)
{
    // <div class="tradeoffer" id="tradeofferid_6189309615">
    if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer"))
    {
        assert(!currentTradeOffer);
        if (auto id=element.getAttribute("id"))
        {
            static const char tradeofferid_prefix[]="tradeofferid_";

            std::string_view number(*id);
            if (number.starts_with(tradeofferid_prefix))
            {
                number.remove_prefix(strlen(tradeofferid_prefix));
                uint64_t tradeOfferId;
                if (SteamBot::parseNumber(number, tradeOfferId))
                {
                    currentTradeOffer=std::make_unique<CurrentTradeOffer>(element);
                    currentTradeOffer->offer->tradeOfferId=tradeOfferId;
                }
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/

void TradeOffersModule::parseIncomingTradeOffserPage(std::string_view html) const
{
    IncomingTradeOffers offers;
    IncomingOffersParser(html, offers).parse();

    BOOST_LOG_TRIVIAL(debug) << "trade offers: " << offers.toJson();
    SteamBot::UI::OutputText() << offers.offers.size() << " incoming trade offers";
}

/************************************************************************/

std::string TradeOffersModule::getIncomingTradeOfferPage() const
{
    auto request=std::make_shared<Request>();
    request->queryMaker=[this](){
        auto url=SteamBot::URLs::getClientCommunityURL();
        url.segments().push_back("tradeoffers");
        url.params().set("l", "english");
        auto request=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        request->cookies=SteamBot::Web::CookieJar::get();
        return request;
    };
    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    return SteamBot::HTTPClient::parseString(*(response->query));
}

/************************************************************************/

void TradeOffersModule::run(SteamBot::Client& client)
{
    waitForLogin();

    auto html=getIncomingTradeOfferPage();
    parseIncomingTradeOffserPage(html);
}

/************************************************************************/

void SteamBot::Modules::TradeOffers::use()
{
}
