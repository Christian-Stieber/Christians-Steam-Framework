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
#include "Modules/AutoAccept.hpp"
#include "Modules/TradeOffers.hpp"
#include "Modules/AutoLoadTradeoffers.hpp"
#include "UI/UI.hpp"
#include "Client/Signal.hpp"
#include "EnumString.hpp"
#include "AcceptTrade.hpp"

/************************************************************************/

typedef SteamBot::TradeOffers::IncomingTradeOffers IncomingTradeOffers;

/************************************************************************/

namespace
{
    class AutoAcceptModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<IncomingTradeOffers::Ptr> incomingOffers;
        SteamBot::Signal::WaiterType stateChangeSignal;

        std::weak_ptr<const IncomingTradeOffers> lastOffers;

        SteamBot::AutoAccept::Items botItems=SteamBot::AutoAccept::Items::None;

    private:
        void handleOffers(const IncomingTradeOffers&);
        void updateAutoLoad(std::unique_ptr<SteamBot::AutoLoadTradeoffers::Enable>&);
        void handleIncoming();

    public:
        AutoAcceptModule()
            : stateChangeSignal(SteamBot::Signal::createWaiter(*waiter))
        {
        }

        virtual ~AutoAcceptModule() =default;

        virtual void init(SteamBot::Client& client) override
        {
            incomingOffers=client.whiteboard.createWaiter<IncomingTradeOffers::Ptr>(*waiter);
        }

        virtual void run(SteamBot::Client&) override;

    public:
        void enableBots(SteamBot::AutoAccept::Items);
    };

    AutoAcceptModule::Init<AutoAcceptModule> init;
}

/************************************************************************/

void AutoAcceptModule::handleOffers(const IncomingTradeOffers& offers)
{
    for (const auto& offer : offers.offers)
    {
        SteamBot::AutoAccept::Items items=SteamBot::AutoAccept::Items::None;
        {
            if (SteamBot::ClientInfo::find(offer.second->partner)!=nullptr)
            {
                items=botItems;
            }
            else
            {
            }
        }

        bool accept=false;
        {
            switch(items)
            {
            case SteamBot::AutoAccept::Items::None:
                accept=false;
                break;

            case SteamBot::AutoAccept::Items::Gifts:
                accept=offer.second->myItems.empty();
                break;

            case SteamBot::AutoAccept::Items::All:
                accept=true;
                break;
            }
        }

        {
            SteamBot::UI::OutputText output;
            output << (accept ? "auto-accepting" : "ignoring");
            output << " tradeoffer id " << toInteger(offer.second->tradeOfferId);
            output << " from " << SteamBot::ClientInfo::prettyName(offer.second->partner);
        }

        if (accept)
        {
            SteamBot::UI::OutputText output;
            output << "accepting " << toInteger(offer.second->tradeOfferId);
            if (SteamBot::acceptTrade(offer.second->tradeOfferId))
            {
                output << " succeeded";
            }
            else
            {
                output << " FAILED";
            }
        }
    }
}

/************************************************************************/

void AutoAcceptModule::updateAutoLoad(std::unique_ptr<SteamBot::AutoLoadTradeoffers::Enable>& autoLoad)
{
    if (stateChangeSignal->testAndClear())
    {
        BOOST_LOG_TRIVIAL(info) << "auto-accept mode: bots=" << SteamBot::enumToStringAlways(botItems);

        if (botItems==SteamBot::AutoAccept::Items::None)
        {
            autoLoad.reset();
        }
        else
        {
            if (!autoLoad)
            {
                autoLoad=std::make_unique<SteamBot::AutoLoadTradeoffers::Enable>();
            }
        }
    }
}

/************************************************************************/

void AutoAcceptModule::handleIncoming()
{
    if (auto incoming=incomingOffers->has())
    {
        if (botItems!=decltype(botItems)::None)
        {
            auto last=lastOffers.lock();
            if (!last || last!=*incoming)
            {
                last.reset();
                handleOffers(**incoming);
                lastOffers=*incoming;
            }
        }
    }
}

/************************************************************************/

void AutoAcceptModule::run(SteamBot::Client& client)
{
    std::unique_ptr<SteamBot::AutoLoadTradeoffers::Enable> autoLoad;
    while (true)
    {
        waiter->wait();

        updateAutoLoad(autoLoad);
        handleIncoming();
    }
}

/************************************************************************/

void AutoAcceptModule::enableBots(SteamBot::AutoAccept::Items items)
{
    if (items!=botItems)
    {
        botItems=items;
        stateChangeSignal->signal();
    }
}

/************************************************************************/

void SteamBot::AutoAccept::enableBots(SteamBot::AutoAccept::Items items)
{
    SteamBot::Client::getClient().getModule<AutoAcceptModule>()->enableBots(items);
}
