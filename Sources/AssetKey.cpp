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

#include "AssetKey.hpp"
#include "Helpers/ParseNumber.hpp"
#include "Helpers/JSON.hpp"

#include <boost/functional/hash.hpp>

/************************************************************************/

typedef SteamBot::AssetKey AssetKey;

/************************************************************************/

AssetKey::AssetKey() =default;
AssetKey::~AssetKey() =default;

/************************************************************************/

bool AssetKey::parseString(std::string_view& string, std::string_view prefix)
{
    if (string.starts_with(prefix))
    {
        string.remove_prefix(prefix.size());
        return true;
    }
    return false;
}

/************************************************************************/
/*
 * "classinfo/753/667924416/667076610"
 * "classinfo/753/667924416/667076610/a:10"
 *
 * Note: we don't parse the "a:10" here. We only require the appId
 * and classId, and an optional instanceId in the string.
 */

bool AssetKey::init(std::string_view& string)
{
    if (parseString(string, "classinfo/") &&
        SteamBot::parseNumberSlash(string, appId) &&
        SteamBot::parseNumberSlash(string, classId))
    {
        SteamBot::parseNumberSlash(string, instanceId);
        return true;
    }
    return false;
}

/************************************************************************/
/*
 * This takes a json from the Steam inventory data, which is slightly
 * different from what our toJson() produces.
 * I'll update this if I need to read our own json as well.
 *
 * {
 *    "appid": 753,
 *    "classid": "5295844374",
 *    "instanceid": "3873503133"
 * }
 */

bool AssetKey::init(const boost::json::value& json)
{
    try
    {
        if (!SteamBot::JSON::optNumber(json, "appid", appId)) return false;
        if (!SteamBot::JSON::optNumber(json, "classid", classId)) return false;
        SteamBot::JSON::optNumber(json, "instanceid", instanceId);
        return true;
    }
    catch(...)
    {
    }
    return false;
}

/************************************************************************/

boost::json::value AssetKey::toJson() const
{
    boost::json::object json;
    if (appId!=SteamBot::AppID::None) json["appId"]=static_cast<std::underlying_type_t<decltype(appId)>>(appId);
    if (classId!=SteamBot::ClassID::None) json["classId"]=static_cast<std::underlying_type_t<decltype(classId)>>(classId);
    if (instanceId!=SteamBot::InstanceID::None) json["instanceId"]=static_cast<std::underlying_type_t<decltype(instanceId)>>(instanceId);
    return json;
}

/************************************************************************/

size_t AssetKey::hash() const
{
    size_t seed=0;
    boost::hash_combine(seed, appId);
    boost::hash_combine(seed, classId);
    boost::hash_combine(seed, instanceId);
    return seed;
}
