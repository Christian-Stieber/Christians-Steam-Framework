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

#include <string_view>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    class AssetKey
    {
    public:
        uint32_t appId=0;
        uint64_t classId=0;
        uint64_t instanceId=0;

    public:
        // Helper functions to parse a "classinfo/..." string
        static bool parseString(std::string_view&, std::string_view);
        static bool parseNumberSlash(std::string_view&, uint32_t&);
        static bool parseNumberSlash(std::string_view&, uint64_t&);

    public:
        AssetKey();
        ~AssetKey();

        boost::json::value toJson() const;

        bool init(std::string_view&);
    };
}
