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

#include "Config.hpp"
#include "ResultCode.hpp"
#include "Client/Module.hpp"
#include "Modules/Login.hpp"
#include "Modules/Connection.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Steam/MsgClientLogon.hpp"
#include "OpenSSL/RSA.hpp"
#include "Base64.hpp"
#include "Steam/MachineInfo.hpp"
#include "Steam/OSType.hpp"
#include "UI/UI.hpp"

#include "steamdatabase/protobufs/steam/steammessages_auth.steamclient.pb.h"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * SteamKit says:
 * Known values are "Unknown", "Client", "Mobile", "Website", "Store", "Community", "Partner", "SteamStats".
 */
static constexpr std::string websiteId{"Client"};

static constexpr auto platformType=EAuthTokenPlatformType::k_EAuthTokenPlatformType_SteamClient;

/************************************************************************/

typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;
typedef SteamBot::Modules::Connection::Whiteboard::LocalEndpoint LocalEndpoint;
typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;

namespace UnifiedMessageClient=SteamBot::Modules::UnifiedMessageClient;
namespace Message=SteamBot::Connection::Message;

typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::GetPasswordRSAPublicKey)> GetPasswordRSAPublicKeyInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::BeginAuthSessionViaCredentials)> BeginAuthSessionViaCredentialsInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::UpdateAuthSessionWithSteamGuardCode)> UpdateAuthSessionWithSteamGuardCodeInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::PollAuthSessionStatus)> PollAuthSessionStatusInfo;

using SteamBot::Connection::Message::Type::ServiceMethodCallFromClientNonAuthed;

/************************************************************************/

class UnsupportedConfirmationsException { };

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
		virtual size_t serialize(SteamBot::Connection::Serializer& serializer) const override
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
    class LoginModule : public SteamBot::Client::Module
    {
    public:
        LoginModule() =default;
        virtual ~LoginModule() =default;

        virtual void run() override;

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
        std::string requestId;

        struct
        {
            EAuthSessionGuardType type=EAuthSessionGuardType::k_EAuthSessionGuardType_None;
            std::string message;
        } confirmation;

    private:
        std::shared_ptr<SteamBot::Waiter> const waiter=SteamBot::Waiter::create();
        std::shared_ptr<SteamBot::UI::GetSteamguardCode> steamguardCodeWaiter;

    private:
        void requestCode();
        void getPollAuthSessionStatus() const;
        void sendGuardCode(std::string);
        void handleEMailCode();
        void getConfirmationType(const BeginAuthSessionViaCredentialsInfo::ResultType&);
        std::shared_ptr<BeginAuthSessionViaCredentialsInfo::ResultType> execute(BeginAuthSessionViaCredentialsInfo::RequestType&&);
        static BeginAuthSessionViaCredentialsInfo::RequestType makeBeginAuthRequest(const LoginModule::PublicKey&);
        static void sendHello();
        PublicKey getPublicKey();
        void beginAuthSession(const LoginModule::PublicKey&);
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
    BeginAuthSessionViaCredentialsInfo::RequestType request;
    request.set_account_name(SteamBot::Config::SteamAccount::get().user);
    request.set_persistence(ESessionPersistence::k_ESessionPersistence_Persistent);
    request.set_website_id(websiteId);
    // guard_data = details.GuardData,
    request.set_encrypted_password(publicKey.encrypt(SteamBot::Config::SteamAccount::get().password));
    request.set_encryption_timestamp(publicKey.timestamp);
    {
        auto deviceDetails=request.mutable_device_details();
        deviceDetails->set_device_friendly_name(Steam::MachineInfo::Provider::getMachineName());
        deviceDetails->set_platform_type(platformType);
        deviceDetails->set_os_type(static_cast<std::underlying_type_t<Steam::OSType>>(Steam::getOSType()));
    }
    return request;
}

/************************************************************************/
/*
 * On a successful response, also sets basic values for the session.
 *
 * On incorrect password, returns an empty ptr.
 */

std::shared_ptr<BeginAuthSessionViaCredentialsInfo::ResultType> LoginModule::execute(BeginAuthSessionViaCredentialsInfo::RequestType&& request)
{
    std::shared_ptr<BeginAuthSessionViaCredentialsInfo::ResultType> content;
    std::shared_ptr<const SteamBot::Modules::UnifiedMessageClient::ServiceMethodResponseMessage> response;
    try
    {
        typedef BeginAuthSessionViaCredentialsInfo::ResultType ResultType;
        response=UnifiedMessageClient::executeFull<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.BeginAuthSessionViaCredentials#1", std::move(request));
    }
    catch(const SteamBot::Modules::UnifiedMessageClient::Error& error)
    {
        if (error!=SteamBot::ResultCode::InvalidPassword)
        {
            throw;
        }
        BOOST_LOG_TRIVIAL(info) << "got an \"invalid password\" response";
        return content;
    }
    content=response->getContent<BeginAuthSessionViaCredentialsInfo::ResultType>();

    if (content->has_request_id())
    {
        requestId=content->request_id();
    }

    {
        auto& whiteboard=getClient().whiteboard;
        if (response->header.proto.has_client_sessionid())
        {
            whiteboard.set(static_cast<SteamBot::Modules::Login::Whiteboard::ClientSessionID>(response->header.proto.client_sessionid()));
        }
        if (content->has_steamid())
        {
            whiteboard.set<SteamBot::Modules::Login::Whiteboard::SteamID>(content->steamid());
        }
        if (content->has_client_id())
        {
            whiteboard.set(static_cast<SteamBot::Modules::Login::Whiteboard::ClientID>(content->client_id()));
        }
    }

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

    if (confirmation.type==k_EAuthSessionGuardType_None)
    {
        throw UnsupportedConfirmationsException();
    }
}

/************************************************************************/

void LoginModule::sendGuardCode(std::string code)
{
    UpdateAuthSessionWithSteamGuardCodeInfo::RequestType request;
    request.set_code_type(confirmation.type);
    request.set_code(std::move(code));
    {
        auto& whiteboard=getClient().whiteboard;
        if (auto steamId=whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>())
        {
            request.set_steamid(steamId->getValue());
        }
        if (auto clientId=whiteboard.has<SteamBot::Modules::Login::Whiteboard::ClientID>())
        {
            request.set_client_id(static_cast<std::underlying_type_t<SteamBot::Modules::Login::Whiteboard::ClientID>>(*clientId));
        }
    }

    typedef UpdateAuthSessionWithSteamGuardCodeInfo::ResultType ResultType;
    std::shared_ptr<ResultType> response;
    try
    {
        response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.UpdateAuthSessionWithSteamGuardCode#1", std::move(request));
    }
    catch(const SteamBot::Modules::UnifiedMessageClient::Error& error)
    {
        if (error!=SteamBot::ResultCode::InvalidLoginAuthCode)
        {
            throw;
        }
    }

    if (response)
    {
        getPollAuthSessionStatus();
    }
    else
    {
        requestCode();
    }
}

/************************************************************************/

void LoginModule::handleEMailCode()
{
    if (steamguardCodeWaiter && steamguardCodeWaiter->isWoken())
    {
        sendGuardCode(steamguardCodeWaiter->fetch());
        steamguardCodeWaiter.reset();
    }
}

/************************************************************************/

void LoginModule::requestCode()
{
    switch(confirmation.type)
    {
    case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceConfirmation:
        assert(false);
        break;

    case EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode:
        assert(false);
        break;

    case EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode:
        steamguardCodeWaiter=SteamBot::UI::GetSteamguardCode::create(waiter);
        break;
    }
}

/************************************************************************/

void LoginModule::beginAuthSession(const LoginModule::PublicKey& publicKey)
{
    auto result=execute(makeBeginAuthRequest(publicKey));
    getConfirmationType(*result);

    requestCode();
}

/************************************************************************/

LoginModule::PublicKey LoginModule::getPublicKey()
{
    GetPasswordRSAPublicKeyInfo::RequestType request;
    request.set_account_name(SteamBot::Config::SteamAccount::get().user);

    typedef GetPasswordRSAPublicKeyInfo::ResultType ResultType;
    auto response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.GetPasswordRSAPublicKey#1", std::move(request));

    PublicKey result;
    if (response->has_publickey_mod()) result.modulus=std::move(*(response->mutable_publickey_mod()));
    if (response->has_publickey_exp()) result.exponent=std::move(*(response->mutable_publickey_exp()));
    if (response->has_timestamp()) result.timestamp=response->timestamp();
    return result;
}

/************************************************************************/
/*
 * ToDo: this has "poll" in the name, beginAuthSession gets a
 * poll-interval...  so, why/how are we supposed to poll?
 */

void LoginModule::getPollAuthSessionStatus() const
{
    PollAuthSessionStatusInfo::RequestType request;
    if (auto clientId=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::ClientID>())
    {
        request.set_client_id(static_cast<std::underlying_type_t<SteamBot::Modules::Login::Whiteboard::ClientID>>(*clientId));
    }
    if (!requestId.empty())
    {
        request.set_request_id(requestId);
    }

    typedef PollAuthSessionStatusInfo::ResultType ResultType;
    auto response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.PollAuthSessionStatus#1", std::move(request));

}

/************************************************************************/

void LoginModule::sendHello()
{
    auto message=std::make_unique<Steam::CMsgClientHelloMessageType>();
    message->content.set_protocol_version(Steam::MsgClientLogon::CurrentProtocol);
    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void LoginModule::run()
{
    setStatus(LoginStatus::LoggedOut);
    getClient().launchFiber("LoginModule::run", [this](){
        auto cancellation=getClient().cancel.registerObject(*waiter);

        std::shared_ptr<SteamBot::Whiteboard::Waiter<ConnectionStatus>> connectionStatus;
        connectionStatus=waiter->createWaiter<decltype(connectionStatus)::element_type>(getClient().whiteboard);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLoggedOffMessageType>> cmsgClientLoggedOffMessage;
        cmsgClientLoggedOffMessage=waiter->createWaiter<decltype(cmsgClientLoggedOffMessage)::element_type>(getClient().messageboard);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLogonResponseMessageType>> cmsgClientLogonResponse;
        cmsgClientLogonResponse=waiter->createWaiter<decltype(cmsgClientLogonResponse)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter->wait();

            while (cmsgClientLoggedOffMessage->fetch())
                ;

            while (cmsgClientLogonResponse->fetch())
                ;

            handleEMailCode();

            if (connectionStatus->get(ConnectionStatus::Disconnected)==ConnectionStatus::Connected)
            {
                if (getClient().whiteboard.get(LoginStatus::LoggedOut)==LoginStatus::LoggedOut)
                {
                    setStatus(LoginStatus::LoggingIn);
                    sendHello();
                    const auto publicKey=getPublicKey();
                    beginAuthSession(publicKey);
                }
            }
        }
    });
}
