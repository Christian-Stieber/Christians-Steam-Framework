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
