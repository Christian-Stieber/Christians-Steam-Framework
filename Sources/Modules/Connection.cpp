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
#include "Asio/Connections.hpp"
#include "Modules/Connection_Internal.hpp"
#include "Connection/Message.hpp"

#include <boost/fiber/barrier.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/trivial.hpp>

#include "Steam/ProtoBuf/steammessages_base.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

/************************************************************************/

typedef SteamBot::Modules::Connection::Internal::HandlerBase HandlerBase;
typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;
typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;

/************************************************************************/

namespace
{
    class ConnectionModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<SendSteamMessage> sendMessageWaiter;

        std::unordered_map<SteamBot::Connection::Message::Type, std::unique_ptr<HandlerBase>> handlers;

    public:
        ConnectionModule();
        virtual ~ConnectionModule();

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;

    private:
        void readPackets(SteamBot::Connections::ConnectResult::element_type*);
        void writePackets(SteamBot::Connections::ConnectResult::element_type*);
        void body();

    public:
        void add(SteamBot::Connection::Message::Type, std::unique_ptr<HandlerBase>&&);

    public:
        void handlePacket(std::span<const std::byte>) const;
    };

    ConnectionModule::Init<ConnectionModule> init;
}

/************************************************************************/

ConnectionModule::ConnectionModule() =default;
ConnectionModule::~ConnectionModule() =default;

/************************************************************************/

void ConnectionModule::add(SteamBot::Connection::Message::Type type, std::unique_ptr<HandlerBase>&& handler)
{
    bool success=handlers.try_emplace(type, std::move(handler)).second;
    if (success)
    {
        BOOST_LOG_TRIVIAL(info) << "added handler for " << SteamBot::enumToStringAlways(type) << " messages";
    }
}

/************************************************************************/

void ConnectionModule::writePackets(SteamBot::Connections::ConnectResult::element_type* const connection)
{
    std::shared_ptr<const SendSteamMessage> message;
    while ((message=sendMessageWaiter->fetch()))
    {
        if (dynamic_cast<const Steam::CMsgClientHeartBeatMessageType*>(message->payload.get())==nullptr)
        {
            BOOST_LOG_TRIVIAL(info) << "sending message: " << boost::typeindex::type_id_runtime(*(message->payload)).pretty_name();
        }
        connection->writePacket(message->payload->Serializeable::serialize());

        typedef SteamBot::Modules::Connection::Whiteboard::LastMessageSent LastMessageSent;
        getClient().whiteboard.set(LastMessageSent(std::chrono::steady_clock::now()));
    }
}

/************************************************************************/

void ConnectionModule::handlePacket(std::span<const std::byte> bytes) const
{
    const auto messageType=SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes);
    auto iterator=handlers.find(messageType);
    if (iterator!=handlers.end())
    {
        BOOST_LOG_TRIVIAL(info) << "received message type " << SteamBot::enumToStringAlways(messageType);
        auto handler=iterator->second.get();
        handler->handle(bytes);
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "ignoring message type " << SteamBot::enumToStringAlways(messageType);
    }
}


/************************************************************************/

void ConnectionModule::readPackets(SteamBot::Connections::ConnectResult::element_type* connection)
{
    while (true)
    {
        auto packet=connection->readPacket();
        if (packet.empty())
        {
            return;
        }
        handlePacket(packet);
    }
}

/************************************************************************/

void ConnectionModule::init(SteamBot::Client& client)
{
    sendMessageWaiter=client.messageboard.createWaiter<SendSteamMessage>(*waiter);
}

/************************************************************************/

static bool updateWhiteboard(SteamBot::Whiteboard& whiteboard, ConnectionStatus status)
{
    if (auto current=whiteboard.has<ConnectionStatus>())
    {
        if (*current==status)
        {
            return false;
        }
    }
    whiteboard.set(status);
    return true;
}

/************************************************************************/

void ConnectionModule::body()
{
    auto& whiteboard=getClient().whiteboard;

    auto connection=SteamBot::Connections::connect(waiter);

    while (true)
    {
        waiter->wait();

        writePackets(connection.get());
        readPackets(connection.get());

        typedef SteamBot::Connections::Connection::Status Status;
        const auto status=connection->getStatus();
        if (status==Status::GotEOF || status==Status::Error)
        {
            break;
        }
        else if (status==Status::Connecting)
        {
            updateWhiteboard(whiteboard, ConnectionStatus::Connecting);
        }
        else if (status==Status::Connected)
        {
            if (updateWhiteboard(whiteboard, ConnectionStatus::Connected))
            {
                connection->storeEndpoint();
                whiteboard.set<SteamBot::Modules::Connection::Whiteboard::LocalEndpoint>(connection->getLocalEndpoint());
            }
        }
    }
}

/************************************************************************/

void ConnectionModule::run(SteamBot::Client& client)
{
    client.whiteboard.set(ConnectionStatus::Disconnected);
    try
    {
        body();
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(debug) << "exception on connection: " << boost::current_exception_diagnostic_information();
    }
    client.whiteboard.set(ConnectionStatus::Disconnected);
    client.whiteboard.clear<SteamBot::Modules::Connection::Whiteboard::LocalEndpoint>();
}

/************************************************************************/

SendSteamMessage::SendSteamMessage(SendSteamMessage::MessageType payload_)
    : payload(std::move(payload_))
{
}

/************************************************************************/

SendSteamMessage::~SendSteamMessage() =default;

/************************************************************************/

void SendSteamMessage::send(SendSteamMessage::MessageType payload)
{
    auto message=std::make_shared<SendSteamMessage>(std::move(payload));
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}

/************************************************************************/

HandlerBase::HandlerBase() =default;
HandlerBase::~HandlerBase() =default;

/************************************************************************/

void HandlerBase::add(SteamBot::Connection::Message::Type type, std::unique_ptr<HandlerBase>&& handler)
{
    SteamBot::Client::getClient().getModule<ConnectionModule>()->add(type, std::move(handler));
}

/************************************************************************/

static void waitMessageDestruct(std::shared_ptr<SteamBot::DestructMonitor>& message)
{
    class Callback : public SteamBot::DestructMonitor::DestructCallback
    {
    public:
        virtual ~Callback() =default;

    private:
        boost::fibers::barrier barrier{2};

    private:
        virtual void call(const SteamBot::DestructMonitor*) override
        {
            barrier.wait();
        }

    public:
        void wait()
        {
            barrier.wait();
        }
    };

    auto callback=std::make_shared<Callback>();
    message->destructCallback=callback;
    message.reset();
    callback->wait();
}

/************************************************************************/
/*
 * This deserializes the packet, and posts it to the messageboard.
 *
 * For specific message types, it will wait until the message is
 * destructed (i.. all recipients have received and processed
 * the message). This ensures that
 *   - for CMsgMulti, the messages are not interlaved with
 *     later messages from the network connection
 *   - for CMsgClientLogonResponse, the message is fully
 *     processed before a potential connection close
 *     is detected and the client is shut down
 */

void HandlerBase::handle(SteamBot::Connection::Base::ConstBytes bytes) const
{
    auto message=send(bytes);
    const auto& messageType=typeid(*message);
    if (messageType==typeid(Steam::CMsgMultiMessageType) ||
        messageType==typeid(Steam::CMsgClientLogonResponseMessageType))
    {
        waitMessageDestruct(message);
    }
}

/************************************************************************/

void SteamBot::Modules::Connection::handlePacket(std::span<const std::byte> bytes)
{
    SteamBot::Client::getClient().getModule<ConnectionModule>()->handlePacket(bytes);
}
