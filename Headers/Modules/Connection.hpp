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

namespace SteamBot
{
    namespace Modules
    {
        namespace Connection
        {
            namespace Messageboard
            {
                class SendSteamMessage
                {
                public:
                    typedef std::unique_ptr<SteamBot::Connection::Message::Base> MessageType;

                public:
                     MessageType payload;

                public:
                    SendSteamMessage(MessageType);
                    ~SendSteamMessage();

                public:
                    static void send(MessageType);
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
        namespace Connection
        {
            namespace Whiteboard
            {
                // Note: LocalEndpoint will be set before ConnectionStatus

                enum class ConnectionStatus {
                    Disconnected,
                    Connecting,
                    Connected
                };

                class LocalEndpoint : public SteamBot::Connection::Endpoint
                {
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
        namespace Connection
        {
            namespace Whiteboard
            {
                class LastMessageSent : public std::chrono::steady_clock::time_point
                {
                public:
                    template <typename ...ARGS> LastMessageSent(ARGS&&... args)
                        : time_point(std::forward<ARGS>(args)...)
                    {
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
        namespace Connection
        {
            void handlePacket(std::span<const std::byte>);
        }
    }
}
