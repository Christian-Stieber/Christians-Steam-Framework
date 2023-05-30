#pragma once

#include "SteamID.hpp"

#include <optional>
#include <utility>
#include <chrono>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Login
        {
            namespace Whiteboard
            {
                enum class LoginStatus {
                    LoggedOut,
                    LoggingIn,
                    LoggedIn
                };

                class SessionInfo
                {
                public:
                    std::optional<SteamBot::SteamID> steamId;
                    std::optional<int32_t> sessionId;
                    std::optional<uint32_t> cellId;
                };

                class HeartbeatInterval : public std::chrono::milliseconds
                {
                public:
                    template <typename ...ARGS> HeartbeatInterval(ARGS&&... args)
                        : std::chrono::milliseconds(std::forward<ARGS>(args)...)
                    {
                    }
                };
            }
        }
    }
}
