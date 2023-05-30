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
