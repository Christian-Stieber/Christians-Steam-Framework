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
 * This is a helper module that auto-loads tradeoffers after
 * notifications have arrived, giving it a short grace time in case
 * more notifications arrive in quick succession.
 *
 * It's mostly used to facilite things like auto-accepting trades,
 * which can then just trigger on the actual trade offer update.
 */

/************************************************************************/

#include "Client/Module.hpp"
#include "Client/Signal.hpp"
#include "Modules/TradeOffers.hpp"
#include "Modules/AutoLoadTradeoffers.hpp"

/************************************************************************/

typedef SteamBot::TradeOffers::Whiteboard::LastIncoming LastIncoming;

/************************************************************************/

namespace
{
    class AutoLoadTradeoffersModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<LastIncoming> incomingNotification;
        SteamBot::Signal::WaiterType stateChangeSignal;

        std::chrono::seconds reload;
        unsigned int enabled=0;

    public:
        AutoLoadTradeoffersModule()
            : stateChangeSignal(SteamBot::Signal::createWaiter(*waiter))
        {
        }

        virtual ~AutoLoadTradeoffersModule() =default;

        virtual void init(SteamBot::Client& client) override
        {
            incomingNotification=client.whiteboard.createWaiter<LastIncoming>(*waiter);
        }

        virtual void run(SteamBot::Client&) override;

    public:
        void enable(bool);
    };

    AutoLoadTradeoffersModule::Init<AutoLoadTradeoffersModule> init;
}

/************************************************************************/

void AutoLoadTradeoffersModule::run(SteamBot::Client&)
{
    while (true)
    {
        stateChangeSignal->testAndClear();

        if (!enabled || reload.count()==0)
        {
            waiter->wait();
        }
        else
        {
            if (!waiter->wait(reload))
            {
                if (enabled)
                {
                    reload=decltype(reload)::zero();
                    SteamBot::TradeOffers::getIncoming();
                    BOOST_LOG_TRIVIAL(debug) << "AutoLoadTradeoffersModule: loaded tradeoffers";
                }
            }
        }

        if (incomingNotification->isWoken())
        {
            // ToDo: can we make use of the data?
            if ([[maybe_unused]] auto notification=incomingNotification->has())
            {
                reload=std::chrono::seconds(15);
            }
            else
            {
                reload=decltype(reload)::zero();
            }
        }
    }
}

/************************************************************************/

void AutoLoadTradeoffersModule::enable(bool state)
{
    if (state)
    {
        if (enabled++==0)
        {
            stateChangeSignal->signal();
        }
    }
    else
    {
        assert(enabled>0);
        if (--enabled==0)
        {
            stateChangeSignal->signal();
        }
    }
}

/************************************************************************/

static void enable(bool state)
{
    SteamBot::Client::getClient().getModule<AutoLoadTradeoffersModule>()->enable(state);
}

/************************************************************************/

SteamBot::AutoLoadTradeoffers::Enable::Enable()
    : owner(SteamBot::Client::getClientShared())
{
    enable(true);
}

/************************************************************************/

SteamBot::AutoLoadTradeoffers::Enable::~Enable()
{
    const auto locked=owner.lock();
    assert(locked);
    assert(locked==SteamBot::Client::getClientShared());

    enable(false);
}
