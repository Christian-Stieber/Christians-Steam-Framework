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

#include "Client/Module.hpp"
#include "Connection/Message.hpp"

/************************************************************************/
/*
 * Note: this needs A LOT more work, to let clients register
 * subscriptions for templated versions of the ServiceMethod message
 * class, link these with the job_name etc.
 *
 * For now, this simple version is merely there so we can see
 * what calls we actually get.
 */

/************************************************************************/

namespace
{
    class UnifiedMessageServerModule : public SteamBot::Client::Module
    {
    public:
        UnifiedMessageServerModule() =default;
        virtual ~UnifiedMessageServerModule() =default;

        virtual void run() override;
    };

    UnifiedMessageServerModule::Init<UnifiedMessageServerModule> init;
}

/************************************************************************/

namespace
{
    class ServiceMethodMessage : public SteamBot::Connection::Message::Base
    {
    public:
        typedef SteamBot::Connection::Message::Header::ProtoBuf headerType;

    public:
        static constexpr auto messageType=SteamBot::Connection::Message::Type::ServiceMethod;

        headerType header;
        // no content... yet?

    public:
        ServiceMethodMessage(std::span<const std::byte> bytes)
            : header(SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes))
        {
            deserialize(bytes);
        }

        virtual ~ServiceMethodMessage() =default;

    public:
        virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
        {
            header.deserialize(deserializer);
        }

        virtual size_t serialize(SteamBot::Connection::Serializer&) const override
        {
            assert(false);
            return 0;
        }

        using Serializeable::deserialize;

    public:
        static void createdWaiter()
        {
            SteamBot::Modules::Connection::Internal::Handler<ServiceMethodMessage>::create();
        }
    };
}

/************************************************************************/

void UnifiedMessageServerModule::run()
{
    getClient().launchFiber("HeartbeatModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<ServiceMethodMessage>> serviceMethodMessage;
        serviceMethodMessage=waiter.createWaiter<decltype(serviceMethodMessage)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter.wait();
            serviceMethodMessage->fetch();
        }
    });
}
