#pragma once

#include "External/MagicEnum/magic_enum.hpp"

/************************************************************************/

namespace SteamBot
{
    enum class PersonaState {
        Offline = 0,
        Online = 1,
        Busy = 2,
        Away = 3,
        Snooze = 4,
        LookingToTrade = 5,
        LookingToPlay = 6,
        Invisible = 7
    };

    enum class EPersonaStateFlag {
        None = 0,
        HasRichPresence = 1,
        InJoinableGame = 2,
        Golden = 4,
        RemotePlayTogether = 8,
        ClientTypeWeb = 256,
        ClientTypeMobile = 512,
        ClientTypeTenfoot = 1024,
        ClientTypeVR = 2048,
        LaunchTypeGamepad = 4096,
        LaunchTypeCompatTool = 8192
    };
}

/************************************************************************/

namespace magic_enum
{
	namespace customize
	{
        template <> struct enum_range<SteamBot::EPersonaStateFlag>
        {
            static constexpr bool is_flags=true;
			static constexpr int min=0;
			static constexpr int max=8192;
        };
    }
}
