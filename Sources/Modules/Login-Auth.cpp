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

#include "steamdatabase/protobufs/steam/steammessages_auth.steamclient.pb.h"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;
typedef SteamBot::Modules::Connection::Whiteboard::LocalEndpoint LocalEndpoint;
typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;

namespace UnifiedMessageClient=SteamBot::Modules::UnifiedMessageClient;
namespace Message=SteamBot::Connection::Message;

typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::GetPasswordRSAPublicKey)> GetPasswordRSAPublicKeyInfo;
typedef UnifiedMessageClient::ProtobufService::Info<decltype(&::Authentication::BeginAuthSessionViaCredentials)> BeginAuthSessionViaCredentialsInfo;

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
        void sendHello();

    private:
        class PublicKey
        {
        public:
            std::string modulus;
            std::string exponent;
            uint64_t timestamp;
        };

        PublicKey getPublicKey();
    };

    LoginModule::Init<LoginModule> init;
}

/************************************************************************/

LoginModule::PublicKey LoginModule::getPublicKey()
{

    GetPasswordRSAPublicKeyInfo::RequestType request;
    request.set_account_name(SteamBot::Config::SteamAccount::get().user);

    using SteamBot::Connection::Message::Type::ServiceMethodCallFromClientNonAuthed;
    typedef GetPasswordRSAPublicKeyInfo::ResultType ResultType;
    auto response=UnifiedMessageClient::execute<ResultType, ServiceMethodCallFromClientNonAuthed>("Authentication.GetPasswordRSAPublicKey#1", std::move(request));

    PublicKey result;
    if (response->has_publickey_mod()) result.modulus=std::move(*(response->mutable_publickey_mod()));
    if (response->has_publickey_exp()) result.exponent=std::move(*(response->mutable_publickey_exp()));
    if (response->has_timestamp()) result.timestamp=response->timestamp();
    return result;
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
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        std::shared_ptr<SteamBot::Whiteboard::Waiter<ConnectionStatus>> connectionStatus;
        connectionStatus=waiter->createWaiter<decltype(connectionStatus)::element_type>(getClient().whiteboard);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLoggedOffMessageType>> cmsgClientLoggedOffMessage;
        cmsgClientLoggedOffMessage=waiter->createWaiter<decltype(cmsgClientLoggedOffMessage)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter->wait();

            while (cmsgClientLoggedOffMessage->fetch())
                ;

            if (connectionStatus->get(ConnectionStatus::Disconnected)==ConnectionStatus::Connected)
            {
                sendHello();
                auto publicKey=getPublicKey();
            }
        }
    });
}
