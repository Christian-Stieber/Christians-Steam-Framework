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
                    std::shared_ptr<const ServiceMethodMessage> message;
                    CONTENT content;

                public:
                    NotificationMessage(std::shared_ptr<const ServiceMethodMessage>&& message_)
                        : message(std::move(message_))
                    {
                        SteamBot::Connection::Deserializer deserializer(message->content);
                        deserializer.getProto(content, deserializer.data.size());
                        if (deserializer.data.size()>0)
                        {
                            BOOST_LOG_TRIVIAL(debug) << "message has " << deserializer.data.size() << " excess bytes at the end";
                        }
                    }
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
            // ToDo: add a check so CONTENT must be a NotificationMessage
            template <typename CONTENT> void registerNotification(std::string name)
            {
                Internal::registerNotification(std::move(name), [](std::shared_ptr<const Internal::ServiceMethodMessage> message) {
                    auto notification=std::make_shared<CONTENT>(std::move(message));
                    SteamBot::Client::getClient().messageboard.send(std::move(notification));
                });
            }
        }
    }
}
