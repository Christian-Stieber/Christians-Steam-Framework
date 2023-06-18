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
#include "ResultCode.hpp"
#include "OpenSSL/Random.hpp"
#include "OpenSSL/RSA.hpp"
#include "OpenSSL/AES.hpp"
#include "Modules/Login.hpp"
#include "Modules/WebSession.hpp"
#include "Web/URLEncode.hpp"
#include "Web/Cookies.hpp"
#include "Base64.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

#include <boost/url/parse.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <queue>

/************************************************************************/
/*
 * This manages an authenticated "session" to query user-specific
 * webpages from the server.
 *
 * Lots of ArchiSteamFarm "insprations" in here.
 */

/************************************************************************/
/*
 * https://partner.steamgames.com/doc/webapi/ISteamUserAuth#AuthenticateUser
 */

/************************************************************************/

typedef Steam::CMsgClientRequestWebAPIAuthenticateUserNonceResponseMessageType NonceMessage;
typedef SteamBot::Modules::WebSession::Messageboard::GetURL GetURL;

/************************************************************************/

namespace
{
    class WebSessionModule : public SteamBot::Client::Module
    {
    private:
        enum class Status {
            None,
            RequestedNonce,
            Failed,
            Ready
        };

        Status status=Status::None;

        std::queue<std::shared_ptr<const GetURL>> requests;

    private:
        std::chrono::steady_clock::time_point timestamp;
        std::string cookies;

    private:
        static std::string createAuthBody(std::string);
        static SteamBot::HTTPClient::Query::QueryPtr performAuthUserRequest(std::string);
        static std::string createCookies(SteamBot::HTTPClient::Query::QueryPtr);
        static void setTimezoneCookie(std::string&);

        void requestNonce();
        void handleRequests();

    public:
        void handle(std::shared_ptr<const NonceMessage>);
        void handle(std::shared_ptr<const GetURL>);

    public:
        WebSessionModule() =default;
        virtual ~WebSessionModule() =default;

        virtual void run() override;
    };

    WebSessionModule::Init<WebSessionModule> init;
}

/************************************************************************/

void WebSessionModule::requestNonce()
{
    if (status==Status::None && !requests.empty())
    {
        auto message=std::make_unique<Steam::CMsgClientRequestWebAPIAuthenticateUserNonceMessageType>();
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
        status=Status::RequestedNonce;
    }
}

/************************************************************************/
/*
 * Create the data for the ISteamUserAuth/AuthenticateUser request
 */

std::string WebSessionModule::createAuthBody(std::string nonce)
{
    // Generate a random 32-byte session key
	std::array<std::byte, 32> sessionKey;
    SteamBot::OpenSSL::makeRandomBytes(sessionKey);

    // RSA encrypt our session key with the public key for the universe we're on
    std::vector<std::byte> encryptedSessionKey;
    {
        SteamBot::OpenSSL::RSACrypto rsa(getClient().universe);
        encryptedSessionKey=rsa.encrypt(sessionKey);
    }

    // Generate login key from the user nonce that we've received from Steam network
    // Note: ASF converts string to UTF8 here, but I don't think I need this

    // AES encrypt our login key with our session key
    std::vector<std::byte> encryptedLoginKey;
    {
        SteamBot::OpenSSL::AESCrypto aes(sessionKey);
        const std::span<const std::byte> bytes(static_cast<const std::byte*>(static_cast<const void*>(nonce.data())), nonce.length());
        encryptedLoginKey=aes.encrypt(bytes);
    }

    // Get the SteamID
    auto steamId=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>();
    assert(steamId!=nullptr);

    // Create the body string
    std::string body;
    SteamBot::Web::formUrlencode(body, "encrypted_loginkey", encryptedLoginKey);
    SteamBot::Web::formUrlencode(body, "sessionkey", encryptedSessionKey);
    SteamBot::Web::formUrlencode(body, "steamid", steamId->getValue());

    return body;
}

/************************************************************************/
/*
 * Performs the ISteamUserAuth/AuthenticateUser with the data, and
 * returns the response. This will block.
 */

SteamBot::HTTPClient::Query::QueryPtr WebSessionModule::performAuthUserRequest(std::string body)
{
    static const auto url=boost::urls::parse_absolute_uri("https://api.steampowered.com/ISteamUserAuth/AuthenticateUser/v1/").value();

    auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::post, url);
    query->request.body()=std::move(body);
    query->request.content_length(query->request.body().size());
    query->request.base().set("Content-Type", "application/x-www-form-urlencoded");

    return SteamBot::HTTPClient::perform(std::move(query));
}

/************************************************************************/

std::string WebSessionModule::createCookies(SteamBot::HTTPClient::Query::QueryPtr query)
{
    std::string cookies;

    auto json=SteamBot::HTTPClient::parseJson(*query);

    boost::json::object& authenticateuser=json.as_object().at("authenticateuser").as_object();
    SteamBot::Web::setCookie(cookies, "steamLogin", authenticateuser.at("token").as_string());
    SteamBot::Web::setCookie(cookies, "steamLoginSecure", authenticateuser.at("tokensecure").as_string());

    {
        auto steamId=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>();
        assert(steamId!=nullptr);

        auto steamIdString=std::to_string(steamId->getValue());
        static auto bytes=static_cast<const std::byte*>(static_cast<const void*>(steamIdString.data()));
        SteamBot::Web::setCookie(cookies, "sessionid", SteamBot::Base64::encode(std::span<const std::byte>(bytes, steamIdString.size())));
    }

    BOOST_LOG_TRIVIAL(debug) << "new cookie string: \"" << cookies << "\"";
    return cookies;
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

void WebSessionModule::handle(std::shared_ptr<const Steam::CMsgClientRequestWebAPIAuthenticateUserNonceResponseMessageType> message)
{
    try
    {
        timestamp=decltype(timestamp)::clock::now();
        cookies.clear();

        if (message->content.has_eresult())
        {
            const auto resultCode=static_cast<SteamBot::ResultCode>(message->content.eresult());
            if (resultCode==decltype(resultCode)::OK)
            {
                if (message->content.has_webapi_authenticate_user_nonce())
                {
                    cookies=createCookies(performAuthUserRequest(createAuthBody(message->content.webapi_authenticate_user_nonce())));
                    status=Status::Ready;
                    return;
                }
            }
        }
        BOOST_LOG_TRIVIAL(error) << "Bad ClientRequestWebAPIAuthenticateUserNonceResponse message";
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "WebSessionModule got an exception: " << boost::current_exception_diagnostic_information();
    }
    status=Status::Failed;
}

/************************************************************************/

void WebSessionModule::handle(std::shared_ptr<const GetURL> message)
{
    requests.push(std::move(message));
}

/************************************************************************/

void WebSessionModule::handleRequests()
{
    while (status==Status::Ready && !requests.empty())
    {
        auto& front=requests.front();

        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, front->url);
        {
            std::string myCookies=cookies;
            setTimezoneCookie(myCookies);
            query->request.base().set("Cookie", std::move(myCookies));
        }

        query=SteamBot::HTTPClient::perform(std::move(query));

        auto reply=std::make_shared<SteamBot::Modules::WebSession::Messageboard::GotURL>();
        reply->initiator=std::move(front);
        reply->query=std::move(query);
        requests.pop();
        getClient().messageboard.send(std::move(reply));
    }
}

/************************************************************************/

void WebSessionModule::run()
{
    getClient().launchFiber("WebSessionModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<NonceMessage>> nonceMessage;
        std::shared_ptr<SteamBot::Messageboard::Waiter<GetURL>> getUrl;
        {
            auto& messageboard=getClient().messageboard;
            nonceMessage=waiter->createWaiter<decltype(nonceMessage)::element_type>(messageboard);
            getUrl=waiter->createWaiter<decltype(getUrl)::element_type>(messageboard);
        }

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter->createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        while (true)
        {
            waiter->wait();

            nonceMessage->handle(this);
            getUrl->handle(this);

            handleRequests();

            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                requestNonce();
            }
        }
    });
}
