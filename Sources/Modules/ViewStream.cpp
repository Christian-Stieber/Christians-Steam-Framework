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

#include "Modules/Connection.hpp"
#include "Client/Module.hpp"
#include "Client/Execute.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/ViewStream.hpp"
#include "HTMLParser/Parser.hpp"
#include "Helpers/JSON.hpp"
#include "UI/UI.hpp"
#include "ResultCode.hpp"
#include "Exception.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    class ErrorException { };
}

/************************************************************************/

namespace
{
    class ViewStream
    {
    private:
        const boost::urls::url url;

        std::string title;
        std::string broadcastSteamId;	// also seen as just "steamid"
        std::string broadcastId;
        std::string viewerToken;
        std::chrono::seconds heartbeatInterval;

    public:
        ViewStream(const boost::urls::url_view& url_)
            : url(url_)
        {
            getBroadcastSteamID();
            getTmpdData();
        }

        ~ViewStream() =default;

    private:
        void getTmpdData();
        void getBroadcastSteamID();

    public:
        void run();
    };
}

/************************************************************************/

namespace
{
    class StreamPageParser : public HTMLParser::Parser
    {
    public:
        std::string data;

    public:
        StreamPageParser(std::string_view html)
            : Parser(html)
        {
        }

        virtual ~StreamPageParser() =default;

    private:
        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            if (element.name=="div")
            {
                if (auto value=element.getAttribute("data-broadcast_available_for_page"))
                {
                    assert(data.empty());
                    data=*value;
                }
            }
        }
    };
}

/************************************************************************/
/*
 * Read the webpage, find the data-broadcast_available_for_page
 * atribute, and get the broadcastStreamID for the first stream.
 */

void ViewStream::getBroadcastSteamID()
{
    boost::json::object json;
    {
        auto request=std::make_shared<Request>();
        request->queryMaker=[this](){
            return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, this->url);
        };
        auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        if (response->query->response.result()!=boost::beast::http::status::ok)
        {
            throw ErrorException();
        }

        auto html=SteamBot::HTTPClient::parseString(*(response->query));
        StreamPageParser parser(html);
        parser.parse();
        if (parser.data.empty())
        {
            throw ErrorException();
        }

        json=boost::json::parse(parser.data).as_object();
    }
    BOOST_LOG_TRIVIAL(info) << "data-broadcast_available_for_page: " << json;

    auto filtered=json.at("filtered").as_array();
    auto first=filtered.at(0);

    if (!SteamBot::JSON::optString(first, "broadcaststeamid", broadcastSteamId))
    {
        throw ErrorException();
    }
}

/************************************************************************/
/*
 * Issue the getbroadcastmpd query to get more stuff
 */

void ViewStream::getTmpdData()
{
    boost::json::value json;
    {
        auto request=std::make_shared<Request>();
        request->queryMaker=[this]() {
            static const boost::urls::url_view myUrl("https://steamcommunity.com/broadcast/getbroadcastmpd/?l=english");
            auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, myUrl);
            query->request.set(boost::beast::http::field::referer, "https://store.steampowered.com/");
            {
                auto params=query->url.params();
                params.set("steamid", broadcastSteamId);
                params.set("broadcastid", "0");
                params.set("viewertoken", "0");
                params.set("watchlocation", "6");
                params.set("sessionid", SteamBot::Modules::WebSession::getSessionId());
            }
            return query;
        };

        auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        if (response->query->response.result()!=boost::beast::http::status::ok)
        {
            throw ErrorException();
        }

        json=SteamBot::HTTPClient::parseJson(*(response->query)).as_object();
    }

    {
        SteamBot::ResultCode result=SteamBot::ResultCode::Invalid;
        if (!SteamBot::JSON::optNumber(json, "eresult", result) || result!=SteamBot::ResultCode::OK) throw ErrorException();
    }

    if (!SteamBot::JSON::optString(json, "broadcastid", broadcastId)) throw ErrorException();
    if (!SteamBot::JSON::optString(json, "viewertoken", viewerToken)) throw ErrorException();

    {
        uint32_t seconds;
        if (!SteamBot::JSON::optNumber(json, "heartbeat_interval", seconds)) throw ErrorException();
        heartbeatInterval=std::chrono::seconds(seconds);
    }

    if (SteamBot::JSON::optString(json, "title", title))
    {
        SteamBot::UI::OutputText() << "stream title: " << title;
    }
}

/************************************************************************/

namespace
{
    class ViewStreamsModule : public SteamBot::Client::Module
    {
    public:
        SteamBot::Execute::WaiterType execute;

    private:
        std::vector<std::unique_ptr<ViewStream>> streams;

    public:
        ViewStreamsModule() =default;
        virtual ~ViewStreamsModule() =default;

    public:
        virtual void init(SteamBot::Client&) override
        {
            execute=SteamBot::Execute::createWaiter(*waiter);
        }

        virtual void run(SteamBot::Client&) override
        {
            waitForLogin();
            while (true)
            {
                waiter->wait();
                execute->run();
            }
        }

    public:
        void startStream(SteamBot::Execute::FunctionBase::Ptr);
    };

    ViewStreamsModule::Init<ViewStreamsModule> init;
}

/************************************************************************/

namespace
{
    class StartFunction : public SteamBot::Execute::FunctionBase
    {
    public:
        std::shared_ptr<ViewStreamsModule> module;
        const boost::urls::url_view& url;
        bool result=false;

    public:
        StartFunction(std::shared_ptr<ViewStreamsModule> module_, const boost::urls::url_view& url_)
            : module(std::move(module_)), url(url_)
        {
        }

        virtual ~StartFunction() =default;

    public:
        virtual void execute(Ptr self) override
        {
            module->startStream(std::move(self));
        }
    };
}

/************************************************************************/

void ViewStreamsModule::startStream(SteamBot::Execute::FunctionBase::Ptr start_)
{
    auto start=std::dynamic_pointer_cast<StartFunction>(start_);
    try
    {
        streams.push_back(std::make_unique<ViewStream>(start->url));
        start->result=true;
    }
    catch(...)
    {
        SteamBot::Exception::log();
    }
    start->complete();
}

/************************************************************************/

bool SteamBot::Modules::ViewStream::start(const boost::urls::url_view& url)
{
    auto module=SteamBot::Client::getClient().getModule<ViewStreamsModule>();
    auto start=std::make_shared<StartFunction>(module, url);
    module->execute->enqueue(start);
    start->wait();
    return start->result;
}
