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
#include "Modules/Heartbeat.hpp"
#include "ResultCode.hpp"
#include "Client/Module.hpp"
#include "Client/ClientInfo.hpp"
#include "Modules/Login.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Steam/MsgClientLogon.hpp"
#include "OpenSSL/RSA.hpp"
#include "Base64.hpp"
#include "Steam/MachineInfo.hpp"
#include "Steam/OSType.hpp"
#include "UI/UI.hpp"
#include "Client/Sleep.hpp"
#include "Helpers/JSON.hpp"
#include "Helpers/Time.hpp"
#include "UI/UI.hpp"
#include "EnumString.hpp"
#include "./Login-Session.hpp"

#include "steamdatabase/protobufs/steam/steammessages_auth.steamclient.pb.h"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <cmath>

/************************************************************************/
/*
 * SteamKit says:
 * Known values are "Unknown", "Client", "Mobile", "Website", "Store", "Community", "Partner", "SteamStats".
 */
static const char websiteId[]="Client";

static constexpr auto platformType=EAuthTokenPlatformType::k_EAuthTokenPlatformType_SteamClient;

/************************************************************************/
/*
 * For the DataFie
 */

struct Keys
{
    inline static const std::string_view SteamGuard="SteamGuard";
    inline static const std::string_view Data="Data";

    inline static const std::string_view Login="Login";
    inline static const std::string_view Refresh="Refresh";
    // inline static const std::string_view Access="Access";
};

/************************************************************************/

typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;
typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;

namespace UnifiedMessageClient=SteamBot::Modules::UnifiedMessageClient;
namespace Message=SteamBot::Connection::Message;

typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::GetPasswordRSAPublicKey)> GetPasswordRSAPublicKeyInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::BeginAuthSessionViaCredentials)> BeginAuthSessionViaCredentialsInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::UpdateAuthSessionWithSteamGuardCode)> UpdateAuthSessionWithSteamGuardCodeInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::PollAuthSessionStatus)> PollAuthSessionStatusInfo;

using SteamBot::Connection::Message::Type::ServiceMethodCallFromClientNonAuthed;

typedef SteamBot::Modules::Login::Internal::CredentialsSession CredentialsSession;

/************************************************************************/

class UnsupportedConfirmationsException { };
class AuthenticationFailedException { };
class InteractiveSessionException { };

/************************************************************************/

namespace
{
    class MsgClientLoggedOff : public Message::Serializeable
	{
	public:
		typedef Message::Message<Message::Type::ClientLoggedOff, Message::Header::Simple, MsgClientLoggedOff> MessageType;

	public:
        SteamBot::ResultCode result;
        uint32_t secMinReconnectHint;	// ???
        uint32_t secMaxReconnectHint;	// ???

	public:
		virtual size_t serialize(SteamBot::Connection::Serializer&) const override
		{
			// we are not sending these
			assert(false);
		}

		virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
		{
			Serializeable::deserialize(deserializer);
			deserializer.get(result, secMinReconnectHint, secMaxReconnectHint);

            BOOST_LOG_TRIVIAL(info) << "result: " << SteamBot::enumToStringAlways(result);
		}
	};
}

/************************************************************************/

namespace
{
    enum class AccountInstance : unsigned int {
        All=0,
        Desktop=1,
        Console=2,
        Web=4
    };
}

/************************************************************************/

namespace
{
    class LoginModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<ConnectionStatus> connectionStatus;
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientLoggedOffMessageType> cmsgClientLoggedOffMessage;
        SteamBot::Messageboard::WaiterType<Steam::CMsgClientLogonResponseMessageType> cmsgClientLogonResponse;

    private:
        std::shared_ptr<CredentialsSession> session;

    public:
        LoginModule() =default;
        virtual ~LoginModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;

    private:
        static void setStatus(LoginStatus status)
        {
            auto& whiteboard=getClient().whiteboard;
            auto current=whiteboard.has<LoginStatus>();
            if (current==nullptr || *current!=status)
            {
                whiteboard.set(status);
            }
        }

    private:
        class PublicKey
        {
        public:
            std::string modulus;
            std::string exponent;
            uint64_t timestamp;

        public:
            std::string encrypt(const std::string&) const;
        };

    private:
        struct
        {
            uint64_t clientId=0;
            uint64_t steamId=0;
            float interval=0;
            int32_t clientSessionId=0;
            std::string requestId;
        } authSessionData;

        std::string refreshToken;
        std::string accessToken;

        struct
        {
            EAuthSessionGuardType type=EAuthSessionGuardType::k_EAuthSessionGuardType_Unknown;
            std::string message;
        } confirmation;

        std::string password;

    public:
        void handle(std::shared_ptr<const Steam::CMsgClientLogonResponseMessageType>);

    private:
        void doLogon();
        void requestCode();
        void doPollAuthSessionStatus();
        void queryPollAuthSessionStatus();
        void sendGuardCode(std::string);
        void getConfirmationType(const BeginAuthSessionViaCredentialsInfo::ResultType&);
        std::shared_ptr<BeginAuthSessionViaCredentialsInfo::ResultType> execute(BeginAuthSessionViaCredentialsInfo::RequestType&&);
        BeginAuthSessionViaCredentialsInfo::RequestType makeBeginAuthRequest(const LoginModule::PublicKey&);
        static void sendHello();
        PublicKey getPublicKey();
        void startAuthSession();
        void loginFailed(const char*);
    };

    LoginModule::Init<LoginModule> init;
}

/************************************************************************/
/*
 * Returns a base64 string
 *
 * Note: SteamKit uses "RSAEncryptionPadding.Pkcs1", while our
 * RSACrypto specifies RSA_PKCS1_OAEP_PADDING.
 * Is this the same thing? We do have a RSA_PKCS1_PADDING as well...
 */

std::string LoginModule::PublicKey::encrypt(const std::string& plainText) const
{
    SteamBot::OpenSSL::RSACrypto rsa(modulus, exponent);
    std::span<const std::byte> plainBytes{static_cast<const std::byte*>(static_cast<const void*>(plainText.data())), plainText.size()};
    auto cipherBytes=rsa.encrypt(plainBytes);
    return SteamBot::Base64::encode(cipherBytes);
}

/************************************************************************/

BeginAuthSessionViaCredentialsInfo::RequestType LoginModule::makeBeginAuthRequest(const LoginModule::PublicKey& publicKey)
{
    assert(!session->password.empty());

    SteamBot::UI::Thread::outputText("sending credentials");
    BeginAuthSessionViaCredentialsInfo::RequestType request;
    request.set_account_name(getClient().getClientInfo().accountName);
    request.set_persistence(ESessionPersistence::k_ESessionPersistence_Persistent);
    request.set_website_id(websiteId);
    {
        getClient().dataFile.examine([&request](const boost::json::value& value) mutable {
            if (auto string=SteamBot::JSON::getItem(value, Keys::SteamGuard, Keys::Data))
            {
                request.set_guard_data(std::string(string->as_string()));
            }
        });
	}
    request.set_encrypted_password(publicKey.encrypt(session->password));
    request.set_encryption_timestamp(publicKey.timestamp);
    {
        auto deviceDetails=request.mutable_device_details();
        deviceDetails->set_device_friendly_name(Steam::MachineInfo::Provider::getMachineName());
        deviceDetails->set_platform_type(platformType);
        deviceDetails->set_os_type(SteamBot::toInteger(Steam::getOSType()));
    }
    return request;
}

/************************************************************************/
/*
 * On a successful response, also sets initial values for the session.
 *
 * On incorrect password, returns an empty ptr.
 */

std::shared_ptr<BeginAuthSessionViaCredentialsInfo::ResultType> LoginModule::execute(BeginAuthSessionViaCredentialsInfo::RequestType&& request)
{
    typedef BeginAuthSessionViaCredentialsInfo::ResultType ResultType;
    std::shared_ptr<ResultType> content;

    std::shared_ptr<const SteamBot::Modules::UnifiedMessageClient::ServiceMethodResponseMessage> response;
    response=UnifiedMessageClient::executeFull<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.BeginAuthSessionViaCredentials#1", std::move(request));

    content=response->getContent<BeginAuthSessionViaCredentialsInfo::ResultType>();

    assert(response->header.proto.has_client_sessionid());
    authSessionData.clientSessionId=response->header.proto.client_sessionid();

    assert(content->has_request_id());
    authSessionData.requestId=content->request_id();

    assert(content->has_steamid());
    authSessionData.steamId=content->steamid();

    assert(content->has_client_id());
    authSessionData.clientId=content->client_id();

    assert(content->has_interval());
    authSessionData.interval=content->interval();

    return content;
}

/************************************************************************/
/*
 * Find the best confirmation type to use.
 */

void LoginModule::getConfirmationType(const BeginAuthSessionViaCredentialsInfo::ResultType& result)
{
    // Most preferred on top
    static constexpr EAuthSessionGuardType preferred[]={
        EAuthSessionGuardType::k_EAuthSessionGuardType_None,
        EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceConfirmation,
        EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode,
        EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode
    };

    int best=sizeof(preferred)/sizeof(preferred[0]);

    for (int i=0; best>0 && i<result.allowed_confirmations_size(); i++)
    {
        auto& allowedConfirmation=result.allowed_confirmations(i);
        if (allowedConfirmation.has_confirmation_type())
        {
            for (int j=0; j<best; j++)
            {
                if (preferred[j]==allowedConfirmation.confirmation_type())
                {
                    confirmation.type=allowedConfirmation.confirmation_type();
                    if (allowedConfirmation.has_associated_message())
                    {
                        confirmation.message=allowedConfirmation.associated_message();
                    }
                    best=j;
                }
            }
        }
    }

    if (confirmation.type==k_EAuthSessionGuardType_Unknown)
    {
        throw UnsupportedConfirmationsException();
    }
}

/************************************************************************/

void LoginModule::sendGuardCode(std::string code)
{
    SteamBot::UI::Thread::outputText("sending guard code");
    UpdateAuthSessionWithSteamGuardCodeInfo::RequestType request;
    request.set_code_type(confirmation.type);
    request.set_code(std::move(code));
    {
        request.set_steamid(authSessionData.steamId);
        request.set_client_id(authSessionData.clientId);
    }

    typedef UpdateAuthSessionWithSteamGuardCodeInfo::ResultType ResultType;
    std::shared_ptr<ResultType> response;
    try
    {
        response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.UpdateAuthSessionWithSteamGuardCode#1", std::move(request));
    }
    catch(const SteamBot::Modules::UnifiedMessageClient::Error& error)
    {
        bool wrongCode=false;

        switch(confirmation.type)
        {
        case EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode:
            wrongCode=(error==SteamBot::ResultCode::InvalidLoginAuthCode);
            break;

        case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode:
            wrongCode=(error==SteamBot::ResultCode::TwoFactorCodeMismatch);
            break;

        default:
            assert(false);
        }

        if (!wrongCode)
        {
            throw;
        }

        session->steamGuardCode.clear();
    }

    if (response)
    {
        doPollAuthSessionStatus();
    }
    else
    {
        requestCode();
    }
}

/************************************************************************/

void LoginModule::requestCode()
{
    switch(confirmation.type)
    {
    case EAuthSessionGuardType::k_EAuthSessionGuardType_None:
        doPollAuthSessionStatus();
        break;

    case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceConfirmation:
        BOOST_LOG_TRIVIAL(info) << "waiting for user to confirm on device...";
        SteamBot::UI::OutputText() << "Please confirm your login on the mobile app";
        doPollAuthSessionStatus();
        break;

    case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode:
    case EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode:
        if (session->steamGuardCode.empty())
        {
            throw InteractiveSessionException();
        }
        else
        {
            sendGuardCode(session->steamGuardCode);
        }
        break;

    default:
        assert(false);
    }
}

/************************************************************************/

void LoginModule::startAuthSession()
{
    auto result=execute(makeBeginAuthRequest(getPublicKey()));
    if (result)
    {
        getConfirmationType(*result);
        requestCode();
    }
}

/************************************************************************/

LoginModule::PublicKey LoginModule::getPublicKey()
{
    GetPasswordRSAPublicKeyInfo::RequestType request;
    request.set_account_name(getClient().getClientInfo().accountName);

    typedef GetPasswordRSAPublicKeyInfo::ResultType ResultType;
    auto response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.GetPasswordRSAPublicKey#1", std::move(request));

    PublicKey result;
    if (response->has_publickey_mod()) result.modulus=std::move(*(response->mutable_publickey_mod()));
    if (response->has_publickey_exp()) result.exponent=std::move(*(response->mutable_publickey_exp()));
    if (response->has_timestamp()) result.timestamp=response->timestamp();
    return result;
}

/************************************************************************/

void LoginModule::queryPollAuthSessionStatus()
{
    PollAuthSessionStatusInfo::RequestType request;
    request.set_client_id(authSessionData.clientId);
    request.set_request_id(authSessionData.requestId);

    typedef PollAuthSessionStatusInfo::ResultType ResultType;
    auto response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.PollAuthSessionStatus#1", std::move(request));

    if (response->has_new_client_id())
    {
        authSessionData.clientId=response->new_client_id();
    }

    if (response->has_new_guard_data())
    {
        getClient().dataFile.update([&response](boost::json::value& json) {
            auto& item=SteamBot::JSON::createItem(json, Keys::SteamGuard, Keys::Data);
            item.emplace_string()=response->new_guard_data();
            return true;
        });
    }

    if (response->has_refresh_token())
    {
        refreshToken=response->refresh_token();
    }
    // We don't need this for anything, since WebSession requests one anyway?
    if (response->has_access_token())
    {
        accessToken=response->access_token();
    }

    getClient().dataFile.update([this](boost::json::value& json) {
        {
            auto& item=SteamBot::JSON::createItem(json, Keys::Login, Keys::Refresh);
            item.emplace_string()=refreshToken;
        }
#if 0
        {
            auto& item=SteamBot::JSON::createItem(json, Keys::Login, Keys::Access);
            item.emplace_string()=accessToken;
        }
#endif
        return true;
    });
}

/************************************************************************/

void LoginModule::doPollAuthSessionStatus()
{
    while (queryPollAuthSessionStatus(), refreshToken.empty())
    {
        switch(confirmation.type)
        {
        case EAuthSessionGuardType::k_EAuthSessionGuardType_None:
        case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode:
        case EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode:
            throw AuthenticationFailedException();
            break;

        case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceConfirmation:
            {
                auto interval=lround(authSessionData.interval*1000);
                SteamBot::sleep(std::chrono::milliseconds(interval));
            }
            break;

        default:
            assert(false);
        }
    }
    doLogon();
}

/************************************************************************/

void LoginModule::sendHello()
{
    auto message=std::make_unique<Steam::CMsgClientHelloMessageType>();
    message->content.set_protocol_version(Steam::MsgClientLogon::CurrentProtocol);
    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void LoginModule::doLogon()
{
    CredentialsSession::remove(std::move(session));

    std::unique_ptr<SteamBot::Modules::Login::ParsedToken> parsedToken;
    try
    {
        parsedToken=std::make_unique<decltype(parsedToken)::element_type>(refreshToken);
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "error while parsing refresh token: "
                                 << boost::current_exception_diagnostic_information();
    }

    {
        SteamBot::UI::OutputText output;
        output << "starting login with key";
        if (parsedToken)
        {
            time_t exp;
            if (SteamBot::JSON::optNumber(parsedToken->toJson(), "exp", exp))
            {
                output << " (expires " << SteamBot::Time::toString(std::chrono::system_clock::from_time_t(exp)) << ")";
            }
        }
    }

    auto message=std::make_unique<Steam::CMsgClientLogonMessageType>();

    message->header.proto.set_client_sessionid(0);
    {
        SteamBot::SteamID steamID;
        steamID.setAccountId(SteamBot::AccountID::None);
        steamID.setAccountInstance(SteamBot::toInteger(AccountInstance::Desktop));
        steamID.setUniverseType(SteamBot::Client::getClient().universe.type);
        steamID.setAccountType(Steam::AccountType::Individual);
        message->header.proto.set_steamid(steamID.getValue());
    }

    message->content.set_protocol_version(Steam::MsgClientLogon::CurrentProtocol);
	message->content.set_cell_id(0);
	message->content.set_client_package_version(1771);
	message->content.set_client_language("english");
	message->content.set_client_os_type(static_cast<uint32_t>(SteamBot::toInteger(Steam::getOSType())));
    message->content.set_should_remember_password(true);
    {
        const auto& machineId=Steam::MachineInfo::MachineID::getSerialized();
        message->content.set_machine_id(machineId.data(), machineId.size());
	}
    message->content.set_account_name(getClient().getClientInfo().accountName);
    message->content.set_eresult_sentryfile(static_cast<int32_t>(toInteger(SteamBot::ResultCode::FileNotFound)));
    message->content.set_steam2_ticket_request(false);
	message->content.set_machine_name(Steam::MachineInfo::Provider::getMachineName());
    message->content.set_supports_rate_limit_response(true);	// ???
    if (!refreshToken.empty())
    {
        BOOST_LOG_TRIVIAL(debug) << "refresh token: " << refreshToken;
        message->content.set_access_token(refreshToken);

        if (parsedToken)
        {
            BOOST_LOG_TRIVIAL(debug) << "refresh token: " << parsedToken->toJson();
        }
    }

    // All this is very weird
	{
		uint32_t ipv4=0;
		{
            typedef SteamBot::Modules::Connection::Whiteboard::LocalEndpoint LocalEndpoint;
            if (auto endpoint=getClient().whiteboard.has<LocalEndpoint>())
            {
                if (endpoint->address.is_v4())
                {
                    static constexpr uint32_t obfuscationMask=0xBAADF00D;
                    const uint32_t address=endpoint->address.to_v4().to_uint();
                    ipv4=address^obfuscationMask;
                }
			}
		}
		if (ipv4!=0)
		{
			message->content.mutable_obfuscated_private_ip()->set_v4(ipv4);
			message->content.set_deprecated_obfustucated_private_ip(ipv4);
		}
	}

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void LoginModule::loginFailed(const char* reason)
{
    SteamBot::UI::OutputText output;
    output << "login " << reason << "; removing login key";
    getClient().dataFile.update([](boost::json::value& json) {
        SteamBot::JSON::eraseItem(json, Keys::Login, Keys::Refresh);
        // SteamBot::JSON::eraseItem(json, Keys::Login, Keys::Access);
        return true;
    });
    getClient().quit(true);
}

/************************************************************************/

void LoginModule::handle(std::shared_ptr<const Steam::CMsgClientLogonResponseMessageType> message)
{
    if (message->content.has_eresult())
    {
        const auto resultCode=static_cast<SteamBot::ResultCode>(message->content.eresult());
        SteamBot::UI::OutputText() << "Login result: " << SteamBot::enumToStringAlways(resultCode);

        switch(resultCode)
        {
        case SteamBot::ResultCode::OK:
            {
                SteamBot::UI::Thread::outputText("login success");

                auto& whiteboard=getClient().whiteboard;
                if (message->header.proto.has_steamid())
                {
                    auto steamId=message->header.proto.steamid();
                    whiteboard.set<SteamBot::Modules::Login::Whiteboard::SteamID>(steamId);
                    getClient().dataFile.update([steamId](boost::json::value& json) {
                        auto& item=SteamBot::JSON::createItem(json, "Info", "SteamID");
                        if (!item.is_null())
                        {
                            if (item.is_number() && item.to_number<decltype(steamId)>()==steamId)
                            {
                                return false;
                            }
                            assert(false);		// shouldn't happen...
                        }
                        item=steamId;
                        return true;
                    });
                }
                if (message->header.proto.has_client_sessionid())
                {
                    whiteboard.set(static_cast<SteamBot::Modules::Login::Whiteboard::ClientSessionID>(message->header.proto.client_sessionid()));
                }
                if (message->content.has_cell_id())
                {
                    whiteboard.set(static_cast<SteamBot::Modules::Login::Whiteboard::CellID>(message->content.cell_id()));
                }
                if (!refreshToken.empty())
                {
                    whiteboard.set(static_cast<SteamBot::Modules::Login::Whiteboard::LoginRefreshToken>(refreshToken));
                }

                if (message->content.has_legacy_out_of_game_heartbeat_seconds())
                {
                    const auto seconds=message->content.legacy_out_of_game_heartbeat_seconds();
                    assert(seconds>0);
                    typedef SteamBot::Modules::Login::Whiteboard::HeartbeatInterval HeartbeatInterval;
                    whiteboard.set<HeartbeatInterval>(std::chrono::seconds(seconds));
                }

                setStatus(LoginStatus::LoggedIn);
            }
            break;

        case SteamBot::ResultCode::InvalidPassword:
        case SteamBot::ResultCode::InvalidSignature:
            // We don't even send passwords, so it must be the refreshToken
            loginFailed("failed");
            break;

        case SteamBot::ResultCode::TryAnotherCM:
        case SteamBot::ResultCode::ServiceUnavailable:
            SteamBot::UI::Thread::outputText("login unavailable");
            getClient().quit(true);
            break;

        case SteamBot::ResultCode::Expired:
            loginFailed("expired");
            break;

        default:
            assert(false);
            break;
        }
    }
}

/************************************************************************/

void LoginModule::init(SteamBot::Client& client)
{
    connectionStatus=client.whiteboard.createWaiter<ConnectionStatus>(*waiter);
    cmsgClientLoggedOffMessage=client.messageboard.createWaiter<Steam::CMsgClientLoggedOffMessageType>(*waiter);
    cmsgClientLogonResponse=client.messageboard.createWaiter<Steam::CMsgClientLogonResponseMessageType>(*waiter);
}

/************************************************************************/

void LoginModule::run(SteamBot::Client& client)
{
    setStatus(LoginStatus::LoggedOut);

    client.dataFile.examine([this](const boost::json::value& value) mutable {
        if (auto string=SteamBot::JSON::getItem(value, Keys::Login, Keys::Refresh))
        {
            refreshToken=string->as_string();
        }
    });

    session=SteamBot::Modules::Login::Internal::CredentialsSession::get(client.getClientInfo());

    while (true)
    {
        try
        {
            waiter->wait();

            cmsgClientLoggedOffMessage->discardMessages();
            cmsgClientLogonResponse->handle(this);

            if (connectionStatus->get(ConnectionStatus::Disconnected)==ConnectionStatus::Connected)
            {
                if (client.whiteboard.get(LoginStatus::LoggedOut)==LoginStatus::LoggedOut)
                {
                    setStatus(LoginStatus::LoggingIn);
                    if (refreshToken.empty())
                    {
                        if (session->password.empty())
                        {
                            throw InteractiveSessionException();
                        }
                        else
                        {
                            sendHello();
                            startAuthSession();
                        }
                    }
                    else
                    {
                        doLogon();
                    }
                }
            }
        }
        catch(const SteamBot::Modules::UnifiedMessageClient::Error& error)
        {
            switch(static_cast<SteamBot::ResultCode>(error))
            {
            case SteamBot::ResultCode::TryAnotherCM:
            case SteamBot::ResultCode::ServiceUnavailable:
                SteamBot::UI::Thread::outputText("login unavailable");
                getClient().quit(true);
                return;

            case SteamBot::ResultCode::InvalidPassword:
                SteamBot::UI::Thread::outputText("invalid password");
                session->password.clear();
                getClient().quit(false);
                return;

            default:
                throw;
            }
        }
        catch(const InteractiveSessionException&)
        {
            CredentialsSession::run(std::move(session));
            return;
        }
    }
}

/************************************************************************/

void SteamBot::Modules::Login::use()
{
    SteamBot::Modules::Heartbeat::use();
}
