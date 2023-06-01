#include "Client/Module.hpp"
#include "ResultCode.hpp"
#include "OpenSSL/Random.hpp"
#include "OpenSSL/RSA.hpp"
#include "OpenSSL/AES.hpp"
#include "Modules/Connection.hpp"
#include "Modules/Login.hpp"
#include "Web/URLEncode.hpp"
#include "HTTPClient.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

#include <boost/url/parse.hpp>

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

namespace
{
    class WebSessionModule : public SteamBot::Client::Module
    {
    private:
        enum class Status {
            None,
            RequestedNonce,
            Ready
        };

        Status status=Status::None;

    private:
        std::string nonce;

    private:
        void performUserAuth();
        void requestNonce();

        bool handle(std::shared_ptr<const Steam::CMsgClientRequestWebAPIAuthenticateUserNonceResponseMessageType>);

    public:
        WebSessionModule() =default;
        virtual ~WebSessionModule() =default;

        virtual void run() override;
    };

    WebSessionModule::Init<WebSessionModule> init;
}

/************************************************************************/

bool WebSessionModule::handle(std::shared_ptr<const Steam::CMsgClientRequestWebAPIAuthenticateUserNonceResponseMessageType> message)
{
    if (message)
    {
        nonce.clear();
        if (message->content.has_eresult())
        {
            const auto resultCode=static_cast<SteamBot::ResultCode>(message->content.eresult());
            if (resultCode==decltype(resultCode)::OK)
            {
                if (message->content.has_webapi_authenticate_user_nonce())
                {
                    nonce=message->content.webapi_authenticate_user_nonce();
                }
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/

void WebSessionModule::requestNonce()
{
    if (status==Status::None)
    {
        auto message=std::make_unique<Steam::CMsgClientRequestWebAPIAuthenticateUserNonceMessageType>();
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
        status=Status::RequestedNonce;
    }
}

/************************************************************************/
/*
 * Call the ISteamUserAuth->AuthenticateUser
 */

void WebSessionModule::performUserAuth()
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
    auto* sessionInfo=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SessionInfo>();
    assert(sessionInfo!=nullptr);
    assert(sessionInfo->steamId);

    // Create the body string
    std::string body;
    SteamBot::Web::formUrlencode(body, "encrypted_loginkey", encryptedLoginKey);
    SteamBot::Web::formUrlencode(body, "sessionkey", encryptedSessionKey);
    SteamBot::Web::formUrlencode(body, "steamid", sessionInfo->steamId->getValue());
    BOOST_LOG_TRIVIAL(debug) << "WebSession authentication body: \"" << body << "\"";

    // Finally, send the thing
    {
        static const auto url=boost::urls::parse_absolute_uri("https://api.steampowered.com/ISteamUserAuth/AuthenticateUser/v1/").value();
        auto request=std::make_shared<SteamBot::HTTPClient::Request>(boost::beast::http::verb::post, url);
        request->body=std::move(body);
        request->contentType="application/x-www-form-urlencoded";

        auto response=SteamBot::HTTPClient::query(std::move(request)).get();
        BOOST_LOG_TRIVIAL(debug) << "WebSession authentication response: " << SteamBot::HTTPClient::parseString(std::move(response));
    }

    status=Status::Ready;
}

/************************************************************************/

void WebSessionModule::run()
{
    getClient().launchFiber("WebSessionModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientRequestWebAPIAuthenticateUserNonceResponseMessageType>> cmsgClientRequestWebAPIAuthenticateUserNonceResponse;
        {
            auto& messageboard=getClient().messageboard;
            cmsgClientRequestWebAPIAuthenticateUserNonceResponse=waiter->createWaiter<decltype(cmsgClientRequestWebAPIAuthenticateUserNonceResponse)::element_type>(messageboard);
        }

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter->createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        while (true)
        {
            waiter->wait();

            while (handle(cmsgClientRequestWebAPIAuthenticateUserNonceResponse->fetch()))
                ;

            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                if (status==Status::None)
                {
                    requestNonce();
                }
            }

            switch(status)
            {
            case Status::None:
                break;

            case Status::RequestedNonce:
                if (!nonce.empty())
                {
                    performUserAuth();
                }
                break;

            case Status::Ready:
                break;
            }
        }
    });
}
