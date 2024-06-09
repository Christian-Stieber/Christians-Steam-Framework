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
 * Some helpers for "flag"-style enums. A "flag"-style enum is an enum
 * based on an unsigned integer type, with values that are powers of 2.
 */

/************************************************************************/
/*
 * Combine multiple flag values into one (by or-ing them).
 */

namespace SteamBot
{
    template <typename ENUM> constexpr ENUM addEnumFlags(ENUM value, ENUM first)
    {
        typedef std::underlying_type_t<ENUM> BaseType;
        return static_cast<ENUM>(static_cast<BaseType>(value) | static_cast<BaseType>(first));
    }

    template <typename ENUM, typename... ARGS> constexpr ENUM addEnumFlags(ENUM value, ENUM first, ARGS... args)
    {
        return addEnumFlags(addEnumFlags(value, first), args...);
    }
}

/************************************************************************/
/*
 * Tests whether a value contains at least one of the flags set in flag.
 */

namespace SteamBot
{
    template <typename ENUM> constexpr bool testEnumFlag(ENUM value, ENUM flag)
    {
        typedef std::underlying_type_t<ENUM> BaseType;
        return (static_cast<BaseType>(value) & static_cast<BaseType>(flag))!=0;
    }
}
