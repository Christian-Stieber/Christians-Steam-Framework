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
        const HTMLParser::Tree::Element* root=nullptr;

        const HTMLParser::Tree::Element* tradeOfferItems=nullptr;
        decltype(TradeOffer::myItems)* tradeOfferItemsList=nullptr;

        std::unique_ptr<TradeOffer> currentTradeOffer;

    public:
        IncomingOffersParser(std::string_view html, IncomingTradeOffers& result_)
            : Parser(html), result(result_)
        {
        }

        virtual ~IncomingOffersParser() =default;

    private:
        bool handleTradeOfferItems_open(const HTMLParser::Tree::Element&);
        bool handleTradeOfferItems_close(const HTMLParser::Tree::Element&);
        bool handleTradeItem(const HTMLParser::Tree::Element&);
        bool handleItemsBanner(const HTMLParser::Tree::Element&);
        bool handleTradePartner(const HTMLParser::Tree::Element&);
        bool handleTradeOffer_open(const HTMLParser::Tree::Element&);
        bool handleTradeOffer_close(const HTMLParser::Tree::Element&);

    private:
        virtual void startElement(const HTMLParser::Tree::Element& element) override
        {
            handleTradeOffer_open(element) || handleTradeOfferItems_open(element) || handleTradePartner(element) || handleTradeItem(element);
        }

        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            handleTradeOffer_close(element) || handleTradeOfferItems_close(element) || handleItemsBanner(element);
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
    json["myItems"]=boost::json::array(myItems.begin(), myItems.end());
    json["theirItems"]=boost::json::array(theirItems.begin(), theirItems.end());
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

bool IncomingOffersParser::handleTradeOfferItems_close(const HTMLParser::Tree::Element& element)
{
    if (&element==tradeOfferItems)
    {
        tradeOfferItems=nullptr;
        tradeOfferItemsList=nullptr;
        return true;
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleTradeOfferItems_open(const HTMLParser::Tree::Element& element)
{
    // <div class="tradeoffer_items primaty">
    // <div class="tradeoffer_items secondary">
    if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items"))
    {
        assert(tradeOfferItems==nullptr && tradeOfferItemsList==nullptr);
        if (currentTradeOffer)
        {
            if (SteamBot::HTML::checkClass(element, "primary"))
            {
                tradeOfferItemsList=&currentTradeOffer->theirItems;
                tradeOfferItems=&element;
            }
            else if (SteamBot::HTML::checkClass(element, "secondary"))
            {
                tradeOfferItemsList=&currentTradeOffer->myItems;
                tradeOfferItems=&element;
            }
            else
            {
                currentTradeOffer.reset();
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleTradeItem(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        // <div class="tradeoffer_item_list">
        //   <div class="trade_item " style="" data-economy-item="classinfo/753/667924416/667076610">

        if (element.name=="div" &&  SteamBot::HTML::checkClass(element, "trade_item"))
        {
            bool killOffer=true;
            if (auto dataEconomyItem=element.getAttribute("data-economy-item"))
            {
                if (auto parent=element.parent)
                {
                    if (parent->name=="div" && SteamBot::HTML::checkClass(*parent, "tradeoffer_item_list"))
                    {
                        if (tradeOfferItemsList!=nullptr)
                        {
                            tradeOfferItemsList->push_back(*dataEconomyItem);
                            killOffer=false;
                        }
                    }
                }
            }
            if (killOffer)
            {
                currentTradeOffer.reset();
            }
            return true;
        }
    }
    return false;
}

/************************************************************************/

bool IncomingOffersParser::handleItemsBanner(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        // <div class="tradeoffer_items_banner in_escrow">
        if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items_banner"))
        {
#if 1
            BOOST_LOG_TRIVIAL(info) << "ignoring trade offer " << currentTradeOffer->tradeOfferId << ": " << SteamBot::HTML::getCleanText(element);
            currentTradeOffer.reset();
#endif
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
                        if (!SteamBot::parseNumber(*dataMiniprofile, currentTradeOffer->partner))
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
    if (root==&element)
    {
        root=nullptr;
        if (currentTradeOffer)
        {
            if (currentTradeOffer->tradeOfferId!=0 && currentTradeOffer->partner!=0 &&
                (currentTradeOffer->myItems.size()!=0 || currentTradeOffer->theirItems.size()!=0))
            {
                result.offers.push_back(std::move(currentTradeOffer));
            }
            currentTradeOffer.reset();
        }
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
        assert(root==nullptr);
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
                    root=&element;
                    currentTradeOffer=std::make_unique<TradeOffer>();
                    currentTradeOffer->tradeOfferId=tradeOfferId;
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
