#pragma once

/************************************************************************/
/*
 * https://github.com/Neargye/magic_enum
 */

#include "External/MagicEnum/magic_enum.hpp"

#include <boost/json.hpp>

/************************************************************************/
/*
 * Returns the name of an enum-value, if possible.
 * Empty string if we don't have a name.
 */

namespace SteamBot
{
	template <typename ENUM> const std::string_view enumToString(ENUM value)
	{
#ifdef MAGIC_ENUM_SUPPORTED
		return magic_enum::enum_name(value);
#else
		return std::string_view();
#endif
	}
}

/************************************************************************/
/*
 * Returns the name of an enum-value, if possible.
 * If we can't get a string, returns the int value as string.
 */

namespace SteamBot
{
	template <typename ENUM> std::string enumToStringAlways(ENUM value)
	{
		std::string result{enumToString(value)};
		if (result.size()==0)
		{
            const auto intValue=static_cast<std::underlying_type_t<ENUM>>(value);
			result=std::to_string(intValue);
		}
		return result;
	}
}

/************************************************************************/
/*
 * Add an enum to a JSON as "<key>".
 *
 * This is meant for informal purposes; it will either add a string
 * "name (value)", or just value as integer.
 */

namespace SteamBot
{
	template <typename ENUM> void enumToJson(boost::json::object& json, const char* key, ENUM value)
	{
		const auto valueString=enumToString(value);
        if (valueString.empty())
        {
            json[key]=static_cast<std::underlying_type_t<ENUM>>(value);
        }
        else
        {
            // Note: we've set the "C" locale at startup, so there should be no issues
            std::string string(magic_enum::enum_type_name<ENUM>());
            string.append("::");
            string.append(valueString);
            string.append(" (");
            string.append(std::to_string(static_cast<std::underlying_type_t<ENUM>>(value)));
            string.append(")");
            json[key]=std::move(string);
        }
	}
}
