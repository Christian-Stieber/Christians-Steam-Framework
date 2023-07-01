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
#include "Modules/DiscoveryQueue.hpp"
#include "Modules/SaleQueue.hpp"
#include "Helpers/HTML.hpp"
#include "UI/UI.hpp"
#include "Client/Sleep.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/url/url_view.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::DiscoveryQueue::Messageboard::ClearQueue ClearQueue;
typedef SteamBot::Modules::DiscoveryQueue::Messageboard::QueueCompleted QueueCompleted;

typedef SteamBot::Modules::SaleQueue::Messageboard::ClearSaleQueues ClearSaleQueues;

/************************************************************************/

ClearSaleQueues::ClearSaleQueues() =default;
ClearSaleQueues::~ClearSaleQueues() =default;

/************************************************************************/

namespace
{
    class SaleQueueModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<ClearSaleQueues> clearSaleQueueWaiter;

    private:
        static bool hasSaleCards();
        void clearSaleQueue();

    public:
        SaleQueueModule()
            : clearSaleQueueWaiter(getClient().messageboard.createWaiter<ClearSaleQueues>(*waiter))
        {
        }

        virtual ~SaleQueueModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    SaleQueueModule::Init<SaleQueueModule> init;
}

/************************************************************************/
/*
 * Again, inspired by ASF
 */

bool SaleQueueModule::hasSaleCards()
{
    class QueuePageParser : public HTMLParser::Parser
    {
    public:
        bool hasCards=false;

    public:
        QueuePageParser(std::string_view html)
            : Parser(html)
        {
        }

        virtual ~QueuePageParser() =default;

    private:
        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            if (element.name=="div" && SteamBot::HTML::checkClass(element, "subtext"))
            {
                auto text=SteamBot::HTML::getCleanText(element);
                BOOST_LOG_TRIVIAL(debug) << "queue page: found text \"" << text << "\"";
                if (text.starts_with("You can get "))
                {
                    hasCards=true;
                }
            }
        }
    };

    auto request=std::make_shared<Request>();
    request->queryMaker=[](){
        static const boost::urls::url_view url("https://store.steampowered.com/explore?l=english");
        return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
    };
    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    QueuePageParser parser(html);
    parser.parse();

    return parser.hasCards;
}

/************************************************************************/

void SaleQueueModule::clearSaleQueue()
{
    auto& client=getClient();

    auto myWaiter=SteamBot::Waiter::create();
    auto cancellation=client.cancel.registerObject(*myWaiter);

    while (true)
    {
        auto queueCompleted=client.messageboard.createWaiter<QueueCompleted>(*myWaiter);

        if (hasSaleCards())
        {
            if (!queueCompleted->fetch())
            {
                getClient().messageboard.send(std::make_shared<ClearQueue>());

                do
                {
                    myWaiter->wait();
                }
                while (!queueCompleted->fetch());
            }
        }
        else
        {
            SteamBot::UI::OutputText() << "no sale queue to be cleared";
            return;
        }

        SteamBot::sleep(std::chrono::seconds(30));
    }
}

/************************************************************************/

void SaleQueueModule::run(SteamBot::Client& client)
{
    waitForLogin();

    while (true)
    {
        waiter->wait();

        if (clearSaleQueueWaiter->fetch())
        {
            clearSaleQueue();

            while (clearSaleQueueWaiter->fetch())
                ;
        }
    }
}
