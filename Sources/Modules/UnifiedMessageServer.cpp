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

#include "Connection/Message.hpp"
#include "Client/Module.hpp"
#include "Modules/UnifiedMessageServer.hpp"

/************************************************************************/
/*
 * We derserialize the ServiceMethod into a "content" that's
 * just a pile of bytes. Our handler for these messages then
 * uses the lookup table from registered target job names to
 * payload data types to produce the real message.
 */

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageServer::Internal::ServiceMethodMessage ServiceMethodMessage;
typedef SteamBot::Modules::UnifiedMessageServer::Internal::MessageHandler MessageHandler;

/************************************************************************/

ServiceMethodMessage::ServiceMethodMessage(std::span<const std::byte> bytes)
    : header(SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes))
{
    deserialize(bytes);
}

ServiceMethodMessage::~ServiceMethodMessage() =default;

/************************************************************************/

void ServiceMethodMessage::deserialize(SteamBot::Connection::Deserializer& deserializer)
{
    header.deserialize(deserializer);

    auto bytes=deserializer.getRemaining();
    content.assign(bytes.begin(), bytes.end());
}

/************************************************************************/

size_t ServiceMethodMessage::serialize(SteamBot::Connection::Serializer&) const
{
    assert(false);
    return 0;
}

/************************************************************************/

void ServiceMethodMessage::createdWaiter()
{
    SteamBot::Modules::Connection::Internal::Handler<ServiceMethodMessage>::create();
}

/************************************************************************/

namespace
{
    class UnifiedMessageServerModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<ServiceMethodMessage> serviceMethodMessage;

        std::unordered_map<std::string, MessageHandler> handlers;

    public:
        void handle(std::shared_ptr<const ServiceMethodMessage>);

    public:
        UnifiedMessageServerModule() =default;
        virtual ~UnifiedMessageServerModule() =default;

        virtual void init(SteamBot::Client& client) override
        {
            serviceMethodMessage=client.messageboard.createWaiter<ServiceMethodMessage>(*waiter);
        }

        virtual void run(SteamBot::Client&) override;

    public:
        void registerHandler(std::string&& name, MessageHandler&& handler)
        {
            auto result=handlers.emplace(std::move(name), std::move(handler));
            assert(result.second);		// no duplicate registrations!
        }
    };

    UnifiedMessageServerModule::Init<UnifiedMessageServerModule> init;
}

/************************************************************************/

void SteamBot::Modules::UnifiedMessageServer::Internal::registerNotification(std::string&& name, MessageHandler handler)
{
    SteamBot::Client::getClient().getModule<UnifiedMessageServerModule>()->registerHandler(std::move(name), std::move(handler));
}

/************************************************************************/

void UnifiedMessageServerModule::handle(std::shared_ptr<const ServiceMethodMessage> message)
{
    if (message->header.proto.has_target_job_name())
    {
        auto iterator=handlers.find(message->header.proto.target_job_name());
        if (iterator!=handlers.end())
        {
            iterator->second(std::move(message));
        }
    }
}

/************************************************************************/

void UnifiedMessageServerModule::run(SteamBot::Client&)
{
    while (true)
    {
        waiter->wait();
        serviceMethodMessage->handle(this);
    }
}
