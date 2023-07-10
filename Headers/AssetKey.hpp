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

#include "MiscIDs.hpp"

#include <string_view>
#include <functional>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    class AssetKey
    {
    public:
        SteamBot::AppID appId=SteamBot::AppID::None;
        SteamBot::ClassID classId=SteamBot::ClassID::None;
        SteamBot::InstanceID instanceId=SteamBot::InstanceID::None;

    public:
        static bool parseString(std::string_view&, std::string_view);

    public:
        size_t hash() const;
        bool operator==(const AssetKey&) const =default;

    private:
        void fromJson(const boost::json::value&);

    public:
        AssetKey();
        AssetKey(const  boost::json::value&);
        virtual ~AssetKey();

        virtual boost::json::value toJson() const;

        bool init(const boost::json::value&);
        bool init(std::string_view&);
    };
}

/************************************************************************/

template <> struct std::hash<SteamBot::AssetKey>
{
    std::size_t operator()(const SteamBot::AssetKey& key) const noexcept
    {
        return key.hash();
    }
};
