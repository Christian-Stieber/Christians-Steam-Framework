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

#pragma once

#include "Connection/Message.hpp"

/************************************************************************/
/*
 * You just call
 *    SteamBot::Modules::UnifiedMessageServer::registerNotification
 * with the content data type and the target job name.
 */

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageServer
        {
            namespace Internal
            {
                class ServiceMethodMessage : public SteamBot::Connection::Message::Base
                {
                public:
                    typedef SteamBot::Connection::Message::Header::ProtoBuf headerType;

                public:
                    static constexpr auto messageType=SteamBot::Connection::Message::Type::ServiceMethod;

                    headerType header;
                    std::vector<std::byte> content;

                public:
                    ServiceMethodMessage(std::span<const std::byte>);
                    virtual ~ServiceMethodMessage();

                public:
                    virtual void deserialize(SteamBot::Connection::Deserializer&) override;
                    virtual size_t serialize(SteamBot::Connection::Serializer&) const override;

                    using Serializeable::deserialize;

                public:
                    static void createdWaiter();
                };
            }
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageServer
        {
            namespace Internal
            {
                typedef std::function<void(std::shared_ptr<const ServiceMethodMessage>)> MessageHandler;
                void registerNotification(std::string&&, MessageHandler);
            }
        }
    }
}

/************************************************************************/
/*
 * We don't need all the baggage from the Connection::Message here.
 * And, I'm too lazy to try and rework the class hierarchy...
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageServer
        {
            namespace Internal
            {
                template <typename CONTENT> class NotificationMessage
                {
                public:
                    SteamBot::Connection::Message::Header::ProtoBuf header;
                    CONTENT content;
                };
            }
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageServer
        {
            // ToDo: add a check so T must be a NotificationMessage
            template <typename T> void registerNotification(std::string name)
            {
                std::string temp=name;
                Internal::registerNotification(std::move(name), [temp=std::move(temp)](std::shared_ptr<const Internal::ServiceMethodMessage>) {
                    BOOST_LOG_TRIVIAL(debug) << "process notification " << temp;
                });
            }
        }
    }
}
