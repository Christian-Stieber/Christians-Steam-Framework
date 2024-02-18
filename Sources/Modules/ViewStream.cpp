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
#include "Helpers/Time.hpp"
#include "UI/UI.hpp"
#include "ResultCode.hpp"
#include "Exception.hpp"
#include "Web/URLEncode.hpp"

/************************************************************************/
/*
 * This module lets you "view" a stream on a steam webpage; this
 * is used to get the item rewards for viewing a stream.
 *
 * From the looks of it, we need to get the webpage containing the
 * stream first, so we can obtain the "steamId" for the stream.
 * Using that, we can query Steam for more information on the
 * stream, which gives us a "broadcastId" and a "viewerToken",
 * as well as a hearrtbeat interval.
 * With all three tokens in hand, we can send "heartbeats" to
 * Steam -- which, to my surprise, seems to be all we need for
 * the items to drop.
 *
 * So, we really don't need to do much "streaming" at all.
 */

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef std::chrono::steady_clock Clock;

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
    public:
        const boost::urls::url url;

    private:
        std::string title;
        std::string broadcastSteamId;	// also seen as just "steamid"
        std::string broadcastId;
        std::string viewerToken;
        std::chrono::seconds heartbeatInterval;

        Clock::time_point startTime;
        Clock::time_point nextReport;
        Clock::time_point nextHeartbeat;

    public:
        ViewStream(const boost::urls::url_view& url_)
            : url(url_)
        {
            SteamBot::UI::OutputText() << "starting stream " << url;
            getBroadcastSteamID();
            getTmpdData();
        }

        ~ViewStream()
        {
            SteamBot::UI::OutputText() << "stopping stream " << url;
        }

    private:
        void getTmpdData();
        void getBroadcastSteamID();

        bool sendHeartbeat();

    public:
        void run();

        decltype(nextHeartbeat) getNextHeartbeat() const
        {
            return nextHeartbeat;
        }

        void doHeartbeat();
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
 * Check whether we need to send a heartbeat, and do it
 */

void ViewStream::doHeartbeat()
{
    auto now=Clock::now();

    if (startTime.time_since_epoch().count()==0)
    {
        startTime=now;
    }

    if (nextHeartbeat<=now)
    {
        if (sendHeartbeat())
        {
            nextHeartbeat=now+heartbeatInterval;
        }
        else
        {
            nextHeartbeat=now+std::chrono::seconds(10);
        }
    }

    if (nextReport<=now)
    {
        SteamBot::UI::OutputText() << "stream \"" << title << "\" (" << broadcastSteamId << ") active for "
                                   << SteamBot::Time::toString(std::chrono::duration_cast<std::chrono::minutes>(now-startTime));
        nextReport=now+std::chrono::minutes(1);
    }
}

/************************************************************************/

bool ViewStream::sendHeartbeat()
{
    auto request=std::make_shared<Request>();
    request->queryMaker=[this](){
        static const boost::urls::url_view myUrl("https://steamcommunity.com/broadcast/heartbeat");
        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::post, myUrl);

        std::string body;
        SteamBot::Web::formUrlencode(body, "steamid", this->broadcastSteamId);
        SteamBot::Web::formUrlencode(body, "broadcastid", this->broadcastId);
        SteamBot::Web::formUrlencode(body, "viewertoken", this->viewerToken);
        query->request.body()=std::move(body);
        query->request.content_length(query->request.body().size());
        query->request.base().set("Content-Type", "application/x-www-form-urlencoded");

        return query;
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        return false;
    }

    return true;
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
            auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, this->url);
            query->url.params().set("l", "english");
            return query;
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

    private:
        Clock::time_point findNextHeartbeat() const;

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
                waiter->wait<Clock>(findNextHeartbeat());

                for (const auto& stream : streams)
                {
                    stream->doHeartbeat();
                }

                execute->run();
            }
        }

    public:
        void startStream(const boost::urls::url_view*);
        void stopStream(const boost::urls::url_view*);
    };

    ViewStreamsModule::Init<ViewStreamsModule> init;
}

/************************************************************************/

Clock::time_point ViewStreamsModule::findNextHeartbeat() const
{
    auto when=Clock::time_point::max();
    for (const auto& stream : streams)
    {
        auto candidate=stream->getNextHeartbeat();
        if (candidate<when)
        {
            when=candidate;
        }
    }
    return when;
}

/************************************************************************/

namespace
{
    class ViewFunction : public SteamBot::Execute::FunctionBase
    {
    public:
        std::shared_ptr<ViewStreamsModule> module;
        enum class Action { Start, Stop } action;
        const boost::urls::url_view* const url;
        bool result=false;

    public:
        ViewFunction(std::shared_ptr<ViewStreamsModule> module_, Action action_, const boost::urls::url_view* url_=nullptr)
            : module(std::move(module_)), action(action_), url(url_)
        {
        }

        virtual ~ViewFunction() =default;

    public:
        virtual void execute(Ptr self) override
        {
            auto function=std::dynamic_pointer_cast<ViewFunction>(self);
            try
            {
                switch(function->action)
                {
                case Action::Start:
                    module->startStream(function->url);
                    function->result=true;
                    break;

                case Action::Stop:
                    module->stopStream(function->url);
                    function->result=true;
                    break;

                default:
                    assert(false);
                    break;
                }
            }
            catch(...)
            {
                SteamBot::Exception::log();
            }
            function->complete();
        }
    };
}

/************************************************************************/

void ViewStreamsModule::startStream(const boost::urls::url_view *url)
{
    assert(url!=nullptr);
    streams.push_back(std::make_unique<ViewStream>(*url));
}

/************************************************************************/

void ViewStreamsModule::stopStream(const boost::urls::url_view *url)
{
    SteamBot::erase(streams, [url](const std::unique_ptr<ViewStream>& stream) {
        if (url!=nullptr)
        {
            return url->compare(stream->url)==0;
        }
        else
        {
            return true;
        }
    });
}

/************************************************************************/

static bool viewAction(ViewFunction::Action action, const boost::urls::url_view* url=nullptr)
{
    auto module=SteamBot::Client::getClient().getModule<ViewStreamsModule>();
    auto function=std::make_shared<ViewFunction>(module, action, url);
    module->execute->enqueue(function);
    function->wait();
    return function->result;
}

/************************************************************************/

bool SteamBot::Modules::ViewStream::start(const boost::urls::url_view& url)
{
    return viewAction(ViewFunction::Action::Start, &url);
}

/************************************************************************/

bool SteamBot::Modules::ViewStream::stop(const boost::urls::url_view& url)
{
    return viewAction(ViewFunction::Action::Stop, &url);
}

/************************************************************************/

bool SteamBot::Modules::ViewStream::stop()
{
    return viewAction(ViewFunction::Action::Stop);
}
