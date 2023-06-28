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
#include "Modules/DiscoveryQueue.hpp"
#include "Helpers/URLs.hpp"
#include "Web/URLEncode.hpp"

#include <boost/url/url_view.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    class DiscoveryQueueModule : public SteamBot::Client::Module
    {
    private:
        enum class State {
            None,
            GenerateQueue
        };

        State state=State::None;

        std::shared_ptr<Request> currentRequest;

    private:
        void generateQueue();

    public:
        void handle(std::shared_ptr<const Response>);

    public:
        DiscoveryQueueModule() =default;
        virtual ~DiscoveryQueueModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    DiscoveryQueueModule::Init<DiscoveryQueueModule> init;
}

/************************************************************************/

void DiscoveryQueueModule::handle(std::shared_ptr<const Response> response)
{
    if (response->initiator==currentRequest)
    {
        BOOST_LOG_TRIVIAL(debug) << SteamBot::HTTPClient::parseString(*(response->query));
    }
}

/************************************************************************/

void DiscoveryQueueModule::generateQueue()
{
    assert(state==State::None);
    state=State::GenerateQueue;

    assert(!currentRequest);
    currentRequest=std::make_shared<Request>();
    currentRequest->queryMaker=[this](){
        static const boost::urls::url_view url("https://store.steampowered.com/explore/generatenewdiscoveryqueue");

        std::string body;
        SteamBot::Web::formUrlencode(body, "queuetype", 0);
        {
            auto cookies=getClient().whiteboard.has<SteamBot::Modules::WebSession::Whiteboard::Cookies>();
            assert(cookies!=nullptr);
            SteamBot::Web::formUrlencode(body, "sessionid", cookies->sessionid);
        }

        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::post, url);
        query->request.body()=std::move(body);
        query->request.content_length(query->request.body().size());
        query->request.base().set("Content-Type", "application/x-www-form-urlencoded");
        return query;
    };

    getClient().messageboard.send(currentRequest);
}

/************************************************************************/

void DiscoveryQueueModule::run(SteamBot::Client& client)
{
    waitForLogin();

    std::shared_ptr<SteamBot::Messageboard::Waiter<Response>> response;
    response=waiter->createWaiter<decltype(response)::element_type>(client.messageboard);

    generateQueue();

    while (true)
    {
        waiter->wait();

        response->handle(this);
    }
}

/************************************************************************/

void SteamBot::Modules::DiscoveryQueue::use()
{
}
