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
#include "Modules/Login.hpp"
#include "Modules/WebSession.hpp"
#include "Web/Cookies.hpp"
#include "Modules/UnifiedMessageClient.hpp"

#include "steamdatabase/protobufs/steam/steammessages_auth.steamclient.pb.h"

#include <boost/url/parse.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <queue>

/************************************************************************/
/*
 * This manages an authenticated "session" to query user-specific
 * webpages from the server.
 *
 * This uses GenerateAccessTokenForApp() from the Authentication
 * service to get an access token for our login, and uses it to create
 * the session cookies.
 */

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::WebSession::Whiteboard::Cookies Cookies;

/************************************************************************/

namespace UnifiedMessageClient=SteamBot::Modules::UnifiedMessageClient;
namespace Message=SteamBot::Connection::Message;

typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::GenerateAccessTokenForApp)> GenerateAccessTokenForAppInfo;

/************************************************************************/

namespace
{
    class WebSessionModule : public SteamBot::Client::Module
    {
    private:
        std::queue<std::shared_ptr<const Request>> requests;
        std::shared_ptr<const Request> forbidden;

        SteamBot::Messageboard::WaiterType<Request> requestWaiter;

    private:
        std::string accessToken;	// don't access this directly, use getAccessToken()
        std::chrono::steady_clock::time_point timestamp;

    private:
        static void setTimezoneCookie(std::string&);
        void setLoginCookie(std::string&);

        const std::string& getAccessToken();
        void handleRequests();

    public:
        void handle(std::shared_ptr<const Request>);

    public:
        WebSessionModule() =default;
        virtual ~WebSessionModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    WebSessionModule::Init<WebSessionModule> init;
}

/************************************************************************/

Request::Request() =default;
Request::~Request() =default;

Response::Response() =default;
Response::~Response() =default;

Cookies::Cookies() =default;
Cookies::~Cookies() =default;

/************************************************************************/
/*
 * If necessary, run GenerateAccessTokenForApp() to get our access token.
 */

const std::string& WebSessionModule::getAccessToken()
{
    if (accessToken.empty())
    {
        GenerateAccessTokenForAppInfo::RequestType request;
        {
            const auto& whiteboard=getClient().whiteboard;
            request.set_refresh_token(whiteboard.get<SteamBot::Modules::Login::Whiteboard::LoginRefreshToken>());
            request.set_steamid(whiteboard.get<SteamBot::Modules::Login::Whiteboard::SteamID>().getValue());
        }

        typedef GenerateAccessTokenForAppInfo::ResultType ResultType;
        auto response=UnifiedMessageClient::execute<ResultType>("Authentication.GenerateAccessTokenForApp#1", std::move(request));

        if (response->has_access_token())
        {
            accessToken=std::move(*(response->mutable_access_token()));
        }
    }

    assert(!accessToken.empty());
    return accessToken;
}

/************************************************************************/
/*
 * Report proper time when doing timezone-based calculations, see
 * setTimezoneCookies() from
 * https://steamcommunity-a.akamaihd.net/public/shared/javascript/shared_global.js
 *
 * Note: I'm not entirely sure whether ASF really does the same as the
 * above, though...
 */

#undef CHRISTIAN_USE_TIMEZONE
#if defined __GNUC__ && __GNUC__ < 13
#else
#define CHRISTIAN_USE_TIMEZONE
#endif

void WebSessionModule::setTimezoneCookie(std::string& cookies)
{
    auto timepoint=std::chrono::system_clock::now();

	int32_t offset=0;
#ifdef CHRISTIAN_USE_TIMEZONE
    // This is untested, for now
	try
	{
		if (auto zone=std::chrono::get_tzdb().current_zone())
		{
			auto info=zone->get_info(timepoint);
			offset=static_cast<decltype(offset)>(info.offset.count());
		}
	}
	catch(const std::runtime_error& exception)
	{
        BOOST_LOG_TRIVIAL(error) << "std::chrono::get_tzdb() failed: " << exception.what();
	}
#else
    auto timestamp=std::chrono::system_clock::to_time_t(timepoint);
    struct tm timedata;
    if (localtime_r(&timestamp, &timedata)!=nullptr)
	{
		offset=timedata.tm_gmtoff;
	}
#endif

    std::string value(std::to_string(offset));
    value.append(",0");

    SteamBot::Web::setCookie(cookies, "timezoneOffset", value);
}

/************************************************************************/

void WebSessionModule::setLoginCookie(std::string& cookies)
{
    std::ostringstream value;

    {
        auto steamId=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>();
        assert(steamId!=nullptr);
        value << steamId->getValue();
    }
    value << "||";
    {
        value << getAccessToken();
    }

    SteamBot::Web::setCookie(cookies, "steamLoginSecure", value.str());
}

/************************************************************************/

void WebSessionModule::handle(std::shared_ptr<const Request> message)
{
    requests.push(std::move(message));
}

/************************************************************************/

void WebSessionModule::handleRequests()
{
    while (!requests.empty())
    {
        auto& front=requests.front();

        auto query=front->queryMaker();
        {
            std::string myCookies;
            setTimezoneCookie(myCookies);
            setLoginCookie(myCookies);

#if 0
            // After 60 seconds, make the cookies invalid
            if (timestamp<std::chrono::steady_clock::now()-std::chrono::seconds(60))
            {
                static const std::string_view cookie="steamLoginSecure=";
                auto position=myCookies.find(cookie);
                assert(position!=std::string::npos);
                assert(position+cookie.size()+1<myCookies.size());
                myCookies[position+cookie.size()]++;
            }
#endif
            query->request.base().set(boost::beast::http::field::cookie, std::move(myCookies));
        }

        query=SteamBot::HTTPClient::perform(std::move(query));

        if (query->response.result()!=boost::beast::http::status::forbidden)
        {
            if (forbidden==front) forbidden.reset();
            assert(!forbidden);

            auto reply=std::make_shared<Response>();
            reply->initiator=std::move(front);
            reply->query=std::move(query);
            requests.pop();
            getClient().messageboard.send(std::move(reply));
        }
        else
        {
            assert(!forbidden);
            forbidden=front;
            accessToken.clear();
        }
    }
}

/************************************************************************/

void WebSessionModule::init(SteamBot::Client& client)
{
    requestWaiter=client.messageboard.createWaiter<Request>(*waiter);
}

/************************************************************************/

void WebSessionModule::run(SteamBot::Client&)
{
    waitForLogin();

    while (true)
    {
        waiter->wait();
        requestWaiter->handle(this);
        handleRequests();
    }
}

/************************************************************************/
/*
 * This is a blocking call.
 */

std::shared_ptr<const Response> SteamBot::Modules::WebSession::makeQuery(std::shared_ptr<Request> request)
{
    auto& client=SteamBot::Client::getClient();

    auto waiter=SteamBot::Waiter::create();
    auto cancellation=client.cancel.registerObject(*waiter);

    std::shared_ptr<SteamBot::Messageboard::Waiter<Response>> response;
    response=waiter->createWaiter<decltype(response)::element_type>(client.messageboard);

    client.messageboard.send(request);

    while (true)
    {
        waiter->wait();

        if (auto message=response->fetch())
        {
            if (message->initiator==request)
            {
                return message;
            }
        }
    }
}
