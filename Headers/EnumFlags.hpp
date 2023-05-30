#pragma once

/************************************************************************/
/*
 * Takes a "flag"-style enum, and adds first/args into the value.
 */

namespace SteamBot
{
    template <typename ENUM> ENUM addEnumFlags(ENUM value, ENUM first)
    {
        typedef std::underlying_type_t<ENUM> BaseType;
        return static_cast<ENUM>(static_cast<BaseType>(value) | static_cast<BaseType>(first));
    }

    template <typename ENUM, typename... ARGS> ENUM addEnumFlags(ENUM value, ENUM first, ARGS... args)
    {
        return addEnumFlags(addEnumFlags(value, first), args...);
    }
}
