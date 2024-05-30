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

/************************************************************************/

#include "Settings.hpp"
#include "Helpers/StringCompare.hpp"

/************************************************************************/

typedef SteamBot::Settings::SettingBool SettingBool;

/************************************************************************/

SettingBool::SettingBool(const SettingBool::InitBase& init_, bool value_)
    : Setting(init_), value(value_)
{
}

SettingBool::~SettingBool() =default;

/************************************************************************/

namespace
{
    struct BoolString
    {
        const std::string_view string;
        const bool value;
    };

    constinit const std::array<BoolString,2*5> boolStrings{{
        { "on", true },
        { "off", false },

        { "enable", true },
        { "disable", false },

        { "yes", true },
        { "no", false },

        { "true", true },
        { "false", false },

        { "1", true },
        { "0", false }
    }};
}

/************************************************************************/

bool SettingBool::setString(std::string_view string)
{
    for (const auto& item : boolStrings)
    {
        if (SteamBot::caseInsensitiveStringCompare_equal(string, item.string))
        {
            value=item.value;
            return true;
        }
    }
    return false;
}

/************************************************************************/

const std::string_view& SettingBool::getString() const
{
    const auto& item=boolStrings[value ? 0 : 1];
    assert(item.value==value);
    return item.string;
}
