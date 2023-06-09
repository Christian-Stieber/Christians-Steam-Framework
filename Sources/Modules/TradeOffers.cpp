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

#include "Modules/UnifiedMessageClient.hpp"
#include "Client/Module.hpp"
#include "Modules/WebSession.hpp"
#include "AssetData.hpp"
#include "Modules/TradeOffers.hpp"
#include "Helpers/URLs.hpp"
#include "Helpers/JSON.hpp"
#include "Web/CookieJar.hpp"
#include "Helpers/HTML.hpp"
#include "Helpers/ParseNumber.hpp"
#include "UI/UI.hpp"
#include "Printable.hpp"
#include "Modules/ClientNotification.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/functional/hash_fwd.hpp>

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::TradeOffers::TradeOffer TradeOffer;
typedef SteamBot::TradeOffers::IncomingTradeOffers IncomingTradeOffers;
typedef SteamBot::TradeOffers::Whiteboard::LastIncoming LastIncoming;

typedef SteamBot::Modules::ClientNotification::Messageboard::ClientNotification ClientNotification;

/************************************************************************/

namespace
{
    class TradeOffersModule : public SteamBot::Client::Module
    {
    private:
        boost::fibers::mutex mutex;		// locked while we loading the tradeoffers

    private:
        SteamBot::Messageboard::WaiterType<ClientNotification> clientNotification;

        std::unordered_set<SteamBot::TradeOfferID> newOffersNotifications;

    private:
        void updateNotification(std::chrono::system_clock::time_point);
        void loadIncoming();
        void getAssetData(IncomingTradeOffers&) const;
        std::unique_ptr<IncomingTradeOffers> parseIncomingTradeOffserPage(std::string_view) const;
        std::string getIncomingTradeOfferPage() const;

    public:
        void handle(std::shared_ptr<const ClientNotification>);

    public:
        TradeOffersModule() =default;
        virtual ~TradeOffersModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;

        std::shared_ptr<const IncomingTradeOffers> getIncoming();
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
        TradeOffer::Item* tradeItem=nullptr;
        decltype(TradeOffer::myItems)* tradeOfferItems=nullptr;

        std::unique_ptr<TradeOffer> currentTradeOffer;

    public:
        IncomingOffersParser(std::string_view html, IncomingTradeOffers& result_)
            : Parser(html), result(result_)
        {
        }

        virtual ~IncomingOffersParser() =default;

    private:
        bool handleCurrencyAmount(const HTMLParser::Tree::Element&);
        bool handleItemsBanner(const HTMLParser::Tree::Element&);
        bool handleTradePartner(const HTMLParser::Tree::Element&);

        std::function<void(const HTMLParser::Tree::Element&)> handleTradeOfferItems(const HTMLParser::Tree::Element&);
        std::function<void(const HTMLParser::Tree::Element&)> handleTradeOffer(const HTMLParser::Tree::Element&);
        std::function<void(const HTMLParser::Tree::Element&)> handleTradeItem(const HTMLParser::Tree::Element&);

    private:
        virtual std::function<void(const HTMLParser::Tree::Element&)> startElement(const HTMLParser::Tree::Element& element) override
        {
            typedef std::function<void(const HTMLParser::Tree::Element&)> ResultType;
            typedef ResultType(IncomingOffersParser::*FunctionType)(const HTMLParser::Tree::Element&);

            static const FunctionType functions[]={
                &IncomingOffersParser::handleTradeItem,
                &IncomingOffersParser::handleTradeOfferItems,
                &IncomingOffersParser::handleTradeOffer
            };

            ResultType result;
            for (auto function : functions)
            {
                result=(this->*function)(element);
                if (result)
                {
                    break;
                }
            }
            return result;
        }

        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            handleTradePartner(element) || handleItemsBanner(element) || handleCurrencyAmount(element);
        }
    };
}

/************************************************************************/

TradeOffer::TradeOffer() =default;
TradeOffer::~TradeOffer() =default;

IncomingTradeOffers::IncomingTradeOffers() =default;
IncomingTradeOffers::~IncomingTradeOffers() =default;

TradeOffer::Item::Item() =default;
TradeOffer::Item::~Item() =default;

/************************************************************************/

bool TradeOffer::Item::init(std::string_view string)
{
    if (SteamBot::AssetKey::init(string))
    {
        if (parseString(string, "a:"))
        {
            if (!parseNumberSlash(string, amount))
            {
                return false;
            }
        }
        if (string.size()==0)
        {
            return true;
        }
    }
    return false;
}

/************************************************************************/

boost::json::value TradeOffer::Item::toJson() const
{
    boost::json::object json;
    json["asset-key"]=SteamBot::AssetKey::toJson();
    if (amount) json["amount"]=amount;
    return json;
}

/************************************************************************/

boost::json::value TradeOffer::toJson() const
{
    boost::json::object json;
    json["tradeOfferId"]=toInteger(tradeOfferId);
    json["partner"]=toInteger(partner);
    {
        boost::json::array array;
        for (const auto& item : myItems)
        {
            array.emplace_back(item->toJson());
        }
        json["myItems"]=std::move(array);
    }
    {
        boost::json::array array;
        for (const auto& item : theirItems)
        {
            array.emplace_back(item->toJson());
        }
        json["theirItems"]=std::move(array);
    }
    return json;
}

/************************************************************************/

boost::json::value IncomingTradeOffers::toJson() const
{
    boost::json::array json;
    for (const auto& offer : offers)
    {
        json.emplace_back(offer.second->toJson());
    }
    return json;
}

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> IncomingOffersParser::handleTradeOfferItems(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> result;

    // <div class="tradeoffer_items primaty">
    // <div class="tradeoffer_items secondary">
    if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items"))
    {
        assert(tradeOfferItems==nullptr);
        if (currentTradeOffer)
        {
            if (SteamBot::HTML::checkClass(element, "primary"))
            {
                tradeOfferItems=&currentTradeOffer->theirItems;
                result=[this](const HTMLParser::Tree::Element& element) { tradeOfferItems=nullptr; };
            }
            else if (SteamBot::HTML::checkClass(element, "secondary"))
            {
                tradeOfferItems=&currentTradeOffer->myItems;
                result=[this](const HTMLParser::Tree::Element& element) { tradeOfferItems=nullptr; };
            }
            else
            {
                currentTradeOffer.reset();
            }
        }
    }

    return result;
}

/************************************************************************/

bool IncomingOffersParser::handleCurrencyAmount(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer && tradeItem!=nullptr)
    {
        // <div class="item_currency_amount">10</div>

        if (element.name=="div" && SteamBot::HTML::checkClass(element, "item_currency_amount"))
        {
            decltype(tradeItem->amount) amount;
            auto string=SteamBot::HTML::getCleanText(element);
            if (SteamBot::parseNumber(string, amount))
            {
                assert(amount==tradeItem->amount);
            }
            return true;
        }
    }
    return false;
}

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> IncomingOffersParser::handleTradeItem(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> result;

    if (currentTradeOffer)
    {
        // <div class="trade_item " style="" data-economy-item="classinfo/753/667924416/667076610">

        if (element.name=="div" && SteamBot::HTML::checkClass(element, "trade_item"))
        {
            assert(tradeOfferItems!=nullptr);
            assert(tradeItem==nullptr);

            if (auto dataEconomyItem=element.getAttribute("data-economy-item"))
            {
                tradeItem=tradeOfferItems->emplace_back(std::make_shared<TradeOffer::Item>()).get();
                if (tradeItem->init(*dataEconomyItem))
                {
                    BOOST_LOG_TRIVIAL(debug) << "" << *dataEconomyItem << " -> " << tradeItem->toJson();
                    result=[this](const HTMLParser::Tree::Element& element) {
                        tradeItem=nullptr;
                    };
                }
                else
                {
                    BOOST_LOG_TRIVIAL(info) << "can't parse asset-string \"" << *dataEconomyItem << "\"";
                    tradeItem=nullptr;
                }
            }

            if (tradeItem==nullptr)
            {
                currentTradeOffer.reset();
            }
        }
    }

    return result;
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
            BOOST_LOG_TRIVIAL(info) << "ignoring trade offer " << toInteger(currentTradeOffer->tradeOfferId) << ": " << SteamBot::HTML::getCleanText(element);
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

std::function<void(const HTMLParser::Tree::Element&)> IncomingOffersParser::handleTradeOffer(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> result;

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
                SteamBot::TradeOfferID tradeOfferId;
                if (SteamBot::parseNumber(number, tradeOfferId))
                {
                    currentTradeOffer=std::make_unique<TradeOffer>();
                    currentTradeOffer->tradeOfferId=tradeOfferId;

                    result=[this](const HTMLParser::Tree::Element& element) {
                        if (currentTradeOffer)
                        {
                            if (currentTradeOffer->tradeOfferId!=SteamBot::TradeOfferID::None &&
                                currentTradeOffer->partner!=SteamBot::AccountID::None &&
                                (currentTradeOffer->myItems.size()!=0 || currentTradeOffer->theirItems.size()!=0))
                            {
                                auto& item=this->result.offers[currentTradeOffer->tradeOfferId];
                                assert(!item);
                                item=std::move(currentTradeOffer);
                            }
                            currentTradeOffer.reset();
                        }
                    };
                }
            }
        }
    }

    return result;
}

/************************************************************************/

std::unique_ptr<IncomingTradeOffers> TradeOffersModule::parseIncomingTradeOffserPage(std::string_view html) const
{
    auto offers=std::make_unique<IncomingTradeOffers>();
    IncomingOffersParser(html, *offers).parse();

    BOOST_LOG_TRIVIAL(debug) << "trade offers: " << offers->toJson();

    return offers;
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

void TradeOffersModule::loadIncoming()
{
    newOffersNotifications.clear();
    getClient().whiteboard.clear<LastIncoming>();
    getClient().whiteboard.clear<IncomingTradeOffers::Ptr>();

    try
    {
        auto html=getIncomingTradeOfferPage();
        auto offers=parseIncomingTradeOffserPage(html);

        {
            SteamBot::AssetData::KeySet keys;
            for (const auto& offer : offers->offers)
            {
                keys.insert(offer.second->myItems.begin(), offer.second->myItems.end());
                keys.insert(offer.second->theirItems.begin(), offer.second->theirItems.end());
            }
            SteamBot::AssetData::fetch(keys);
        }

        getClient().whiteboard.set<IncomingTradeOffers::Ptr>(std::move(offers));
    }
    catch(...)
    {
    }
}

/************************************************************************/

std::shared_ptr<const IncomingTradeOffers> TradeOffersModule::getIncoming()
{
    auto& whiteboard=getClient().whiteboard;

    std::lock_guard<decltype(mutex)> lock(mutex);
    {
        auto offers=whiteboard.has<IncomingTradeOffers::Ptr>();
        if (offers==nullptr || !newOffersNotifications.empty())
        {
            loadIncoming();
        }
        return whiteboard.get<IncomingTradeOffers::Ptr>(nullptr);
    }
}

/************************************************************************/

void TradeOffersModule::handle(std::shared_ptr<const ClientNotification> notification)
{
    if (notification->type==ClientNotification::Type::TradeOffer)
    {
        auto tradeOfferId=SteamBot::JSON::toNumber<SteamBot::TradeOfferID>(notification->body.at("tradeoffer_id"));
        auto sender=SteamBot::JSON::toNumber<SteamBot::AccountID>(notification->body.at("sender"));
        if (auto offers=getClient().whiteboard.has<IncomingTradeOffers::Ptr>())
        {
            auto iterator=(*offers)->offers.find(tradeOfferId);
            if (iterator!=(*offers)->offers.end())
            {
                assert(iterator->second.get()->tradeOfferId==tradeOfferId);
                assert(iterator->second.get()->partner==sender);
                return;
            }
        }

        if (newOffersNotifications.insert(tradeOfferId).second)
        {
            SteamBot::UI::OutputText() << "new trade offer id " << toInteger(tradeOfferId) << " from " << SteamBot::ClientInfo::prettyName(sender);
            getClient().whiteboard.set<LastIncoming>(std::chrono::system_clock::now());
        }
    }
}

/************************************************************************/

void TradeOffersModule::init(SteamBot::Client& client)
{
    clientNotification=client.messageboard.createWaiter<ClientNotification>(*waiter);
}

/************************************************************************/

void TradeOffersModule::run(SteamBot::Client& client)
{
    SteamBot::Modules::ClientNotification::use();

    waitForLogin();

    while (true)
    {
        waiter->wait();
        clientNotification->handle(this);
    }
}

/************************************************************************/
/*
 * Returns the current trade offers, possibly loading it if necessary.
 */

std::shared_ptr<const IncomingTradeOffers> SteamBot::TradeOffers::getIncoming()
{
    return SteamBot::Client::getClient().getModule<TradeOffersModule>()->getIncoming();
}
