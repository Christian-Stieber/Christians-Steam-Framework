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

#include <cstdint>
#include <type_traits>
#include <cassert>
#include <ostream>

/************************************************************************/

namespace SteamBot
{
    enum class AppID : int32_t {
        None=-1,
        Steam=753
    };

    enum class PackageID : uint32_t {
        Steam=0		// I think?
    };

    enum class AssetID : uint64_t {
        None=0
    };

    enum class ContextID : int32_t {
        None=-1
    };

    enum class ClassID : uint64_t {
        None=0
    };

    enum class InstanceID : uint64_t {
        None=0
    };

    // also known as "Steam32" ID
    enum class AccountID : uint32_t {
        None=0
    };

    enum class TradeOfferID : uint64_t {
        None=0
    };

    enum class NotificationID : uint64_t {
        None=0
    };

}

/************************************************************************/

namespace SteamBot
{
    template <typename T> constexpr auto toInteger(T number) requires(std::is_enum_v<T>)
    {
        return static_cast<std::underlying_type_t<T>>(number);
    }
}

/************************************************************************/

namespace SteamBot
{
    template <typename T> auto toUnsignedInteger(T number) requires(std::is_enum_v<T> && std::is_signed_v<std::underlying_type_t<T>>)
    {
        const auto value=toInteger(number);
        assert(value>=0);
        return std::make_unsigned_t<std::underlying_type_t<T>>(value);
    }
}

/************************************************************************/
/*
 * Output an AppID as "number (name)"
 *
 * Note: this accesses OwnedGames, so it only works on client threads.
 * ToDo: make it use AppInfo to get the name instead.
 */

namespace SteamBot
{
    std::ostream& operator<<(std::ostream&, AppID);
}
