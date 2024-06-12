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

#include "Cloud.hpp"
#include "Helpers/StringCompare.hpp"
#include "EnumFlags.hpp"

#include <array>

/************************************************************************/
/*
 * https://partner.steamgames.com/doc/webapi/icloudservice
 * has the platforms listed under BeginHTTPUpload
 */

/************************************************************************/

typedef SteamBot::Cloud::Platform Platform;

/************************************************************************/

namespace
{
    struct PlatformName
    {
        const Platform platform;
        const std::string_view name;

        constexpr PlatformName(Platform platform_, std::string_view name_)
            : platform(platform_), name(name_)
        {
        }
    };

    constinit const std::array platformNames{
        PlatformName{Platform::All, "All"},
        PlatformName{Platform::Windows, "Windows"},
        PlatformName{Platform::MacOS, "MacOS"},
        PlatformName{Platform::Linux, "Linux"},
        PlatformName{Platform::Android, "Android"},
        PlatformName{Platform::iOS, "iPhoneOS"},
        PlatformName{Platform::Switch, "Switch"}
    };
}

/************************************************************************/

Platform SteamBot::Cloud::getPlatform(std::string_view name)
{
    for (const PlatformName& platformName: platformNames)
    {
        if (caseInsensitiveStringCompare_equal(name, platformName.name))
        {
            return platformName.platform;
        }
    }
    return Platform::None;
}

/************************************************************************/

std::vector<std::string_view> SteamBot::Cloud::getStrings(Platform platforms)
{
    std::vector<std::string_view> result;
    result.reserve(platformNames.size());
    for (const PlatformName& platformName: platformNames)
    {
        if (SteamBot::testEnumFlag(platforms, platformName.platform))
        {
            result.push_back(platformName.name);
        }
    }
    return result;
}
