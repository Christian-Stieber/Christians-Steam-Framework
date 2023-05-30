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

#include "BitField.hpp"
#include "MiscIDs.hpp"

/************************************************************************/
/*
 * This GameID implementation is incomplete; check
 * SteamKit2/SteamKit2/Types/GameID.cs for the missing features.
 */

namespace SteamBot
{
	class GameID : public BitField<uint64_t>
    {
    public:
        enum class AppType {
            App = 0,		// A Steam application.
            GameMod = 1,	// A game modification.
            Shortcut = 2,	// A shortcut to a program.
            P2P = 3			// A peer-to-peer file.
        };

    public:
        using BitField::BitField;

        STEAM_BITFIELD_MAKE_ACCESSORS(AppID, AppId, 0, 24);
        STEAM_BITFIELD_MAKE_ACCESSORS(AppType, AppType, 24, 8);
    };
}
