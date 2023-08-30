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
#include "Helpers/URLs.hpp"
#include "Helpers/JSON.hpp"
#include "Web/CookieJar.hpp"
#include "Helpers/ParseNumber.hpp"
#include "UI/UI.hpp"
#include "Printable.hpp"
#include "Modules/ClientNotification.hpp"
#include "EnumString.hpp"

#include "./TradeOffers.hpp"

#include <boost/functional/hash_fwd.hpp>

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::ClientNotification::Messageboard::ClientNotification ClientNotification;

typedef SteamBot::TradeOffers::Whiteboard::LastIncoming LastIncoming;
typedef SteamBot::TradeOffers::IncomingTradeOffers IncomingTradeOffers;
typedef SteamBot::TradeOffers::OutgoingTradeOffers OutgoingTradeOffers;

/************************************************************************/

namespace
{
    class TradeOffersModule : public SteamBot::Client::Module
    {
    private:
    private:
        boost::fibers::mutex mutex;		// locked while we loading the tradeoffers

    private:
        SteamBot::Messageboard::WaiterType<ClientNotification> clientNotification;

        std::unordered_set<SteamBot::TradeOfferID> newOffersNotifications;

    private:
        void updateNotification(std::chrono::system_clock::time_point);
        bool loadOffers(TradeOffers&);
        void getTradeOfferPage(TradeOffers&) const;

    public:
        void handle(std::shared_ptr<const ClientNotification>);

    public:
        TradeOffersModule() =default;
        virtual ~TradeOffersModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;

        std::shared_ptr<const IncomingTradeOffers> getIncoming();
        std::shared_ptr<const OutgoingTradeOffers> getOutgoing();
    };

    TradeOffersModule::Init<TradeOffersModule> init;
}

/************************************************************************/

TradeOffer::TradeOffer() =default;
TradeOffer::~TradeOffer() =default;

TradeOffers::TradeOffers(TradeOffers::Direction direction_) : direction(direction_) { }
TradeOffers::~TradeOffers() =default;

TradeOffer::Item::Item() =default;
TradeOffer::Item::~Item() =default;

IncomingTradeOffers::IncomingTradeOffers() : TradeOffers(Direction::Incoming) { }
IncomingTradeOffers::~IncomingTradeOffers() =default;

OutgoingTradeOffers::OutgoingTradeOffers() : TradeOffers(Direction::Outgoing) { }
OutgoingTradeOffers::~OutgoingTradeOffers() =default;

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

boost::json::value TradeOffers::toJson() const
{
    boost::json::array json;
    for (const auto& offer : offers)
    {
        json.emplace_back(offer.second->toJson());
    }
    return json;
}

/************************************************************************/

void TradeOffersModule::getTradeOfferPage(TradeOffers& offers) const
{
    auto request=std::make_shared<Request>();
    request->queryMaker=[this, &offers](){
        auto url=SteamBot::URLs::getClientCommunityURL();
        url.segments().push_back("tradeoffers");
        if (offers.direction==TradeOffers::Direction::Outgoing)
        {
            url.segments().push_back("sent");
        }
        url.params().set("l", "english");
        auto myRequest=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        myRequest->cookies=SteamBot::Web::CookieJar::get();
        return myRequest;
    };
    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    std::string html=SteamBot::HTTPClient::parseString(*(response->query));

    SteamBot::Modules::TradeOffers::Internal::Parser(html, offers).parse();
    BOOST_LOG_TRIVIAL(debug) << "trade offers (" << SteamBot::enumToString(offers.direction) << "): " << offers.toJson();
}

/************************************************************************/

bool TradeOffersModule::loadOffers(TradeOffers& offers)
{
    try
    {
        getTradeOfferPage(offers);

        {
            SteamBot::AssetData::KeySet keys;
            for (const auto& offer : offers.offers)
            {
                keys.insert(offer.second->myItems.begin(), offer.second->myItems.end());
                keys.insert(offer.second->theirItems.begin(), offer.second->theirItems.end());
            }
            SteamBot::AssetData::fetch(keys);
        }

        return true;
    }
    catch(...)
    {
        return false;
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
            newOffersNotifications.clear();
            getClient().whiteboard.clear<LastIncoming>();
            getClient().whiteboard.clear<IncomingTradeOffers::Ptr>();

            try
            {
                auto newOffers=std::make_shared<IncomingTradeOffers>();
                if (loadOffers(*newOffers))
                {
                    getClient().whiteboard.set<IncomingTradeOffers::Ptr>(std::move(newOffers));
                }
            }
            catch(...)
            {
            }
        }
        return whiteboard.get<IncomingTradeOffers::Ptr>(nullptr);
    }
}

/************************************************************************/

std::shared_ptr<const OutgoingTradeOffers> TradeOffersModule::getOutgoing()
{
    auto& whiteboard=getClient().whiteboard;

    std::lock_guard<decltype(mutex)> lock(mutex);
    {
        auto offers=whiteboard.has<OutgoingTradeOffers::Ptr>();
        if (offers==nullptr || !newOffersNotifications.empty())
        {
            getClient().whiteboard.clear<OutgoingTradeOffers::Ptr>();
            try
            {
                auto newOffers=std::make_shared<OutgoingTradeOffers>();
                if (loadOffers(*newOffers))
                {
                    getClient().whiteboard.set<OutgoingTradeOffers::Ptr>(std::move(newOffers));
                }
            }
            catch(...)
            {
            }
        }
        return whiteboard.get<OutgoingTradeOffers::Ptr>(nullptr);
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

void TradeOffersModule::run(SteamBot::Client&)
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

/************************************************************************/
/*
 * Returns the current trade offers, possibly loading it if necessary.
 */

std::shared_ptr<const OutgoingTradeOffers> SteamBot::TradeOffers::getOutgoing()
{
    return SteamBot::Client::getClient().getModule<TradeOffersModule>()->getOutgoing();
}
