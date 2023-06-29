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
#include "UI/UI.hpp"
#include "MiscIDs.hpp"

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
        std::vector<SteamBot::AppID> processGenerateResponse(std::shared_ptr<const Response>) const;
        std::shared_ptr<Request> makeGenerateRequest() const;
        void processQueue() const;

    public:
        DiscoveryQueueModule() =default;
        virtual ~DiscoveryQueueModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    DiscoveryQueueModule::Init<DiscoveryQueueModule> init;
}

/************************************************************************/

std::vector<SteamBot::AppID> DiscoveryQueueModule::processGenerateResponse(std::shared_ptr<const Response> response) const
{
    std::vector<SteamBot::AppID> queue;

    auto data=SteamBot::HTTPClient::parseJson(*(response->query));
    BOOST_LOG_TRIVIAL(debug) << data;
    try
    {
        {
            auto& array=data.at("queue").as_array();
            queue.reserve(array.size());
            for (auto iterator=array.crbegin(); iterator!=array.crend(); ++iterator)
            {
                auto value=iterator->to_number<std::underlying_type_t<SteamBot::AppID>>();
                queue.push_back(static_cast<SteamBot::AppID>(value));
            }
        }

        {
            auto& appData=data.at("rgAppData").as_object();
            assert(appData.size()==queue.size());

            SteamBot::UI::OutputText output;
            output << "discovery queue: ";
            const char* separator="";
            for (const auto& item : appData)
            {
                output << separator << item.key() << " (" << item.value().at("name").as_string() << ")";
                separator=", ";
            }
        }
    }
    catch(std::invalid_argument&)
    {
        queue.clear();
    }
    catch(std::out_of_range&)
    {
        queue.clear();
    }

    return queue;
}

/************************************************************************/

std::shared_ptr<Request> DiscoveryQueueModule::makeGenerateRequest() const
{
    auto request=std::make_shared<Request>();
    request->queryMaker=[this](){
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
    return request;
}

/************************************************************************/

void DiscoveryQueueModule::processQueue() const
{
    auto generateRequest=makeGenerateRequest();
    auto geneerateResponse=SteamBot::Modules::WebSession::makeQuery(std::move(generateRequest));
    auto queue=processGenerateResponse(std::move(geneerateResponse));
}

/************************************************************************/

void DiscoveryQueueModule::run(SteamBot::Client& client)
{
    waitForLogin();

    processQueue();

    while (true)
    {
        waiter->wait();
    }
}

/************************************************************************/

void SteamBot::Modules::DiscoveryQueue::use()
{
}
