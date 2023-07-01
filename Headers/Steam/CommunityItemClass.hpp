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

/************************************************************************/
/*
 * SteamKit, Enums.cs
 */

namespace SteamBot
{
    enum class CommunityItemClass : int32_t {
        Invalid = 0,
        Badge = 1,
        GameCard = 2,
        ProfileBackground = 3,
        Emoticon = 4,
        BoosterPack = 5,
        Consumable = 6,
        GameGoo = 7,
        ProfileModifier = 8,
        Scene = 9,
        SalienItem = 10,
        Sticker = 11,
        ChatEffect = 12,
        MiniProfileBackground = 13,
        AvatarFrame = 14,
        AnimatedAvatar = 15,
        SteamDeckKeyboardSkin = 16,
        SteamDeckStartupMovie = 17
    };
}
