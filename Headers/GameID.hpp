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
