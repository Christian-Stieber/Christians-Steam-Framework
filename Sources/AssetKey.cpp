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

/************************************************************************/

typedef SteamBot::AssetKey AssetKey;

/************************************************************************/

AssetKey::AssetKey() =default;
AssetKey::~AssetKey() =default;

/************************************************************************/
/*
 * Parse a number. Parsing must stop on end-of-string or a "/"
 * character, which is skipped.
 */

template <typename T> static bool parseNumberSlash(std::string_view& string, T& number)
{
    if (SteamBot::parseNumberPrefix(string, number))
    {
        if (string.size()==0)
        {
            return true;
        }
        if (string.front()=='/')
        {
            string.remove_prefix(1);
            return true;
        }
    }
    return false;
}

/************************************************************************/

bool AssetKey::parseNumberSlash(std::string_view& string, uint64_t& number)
{
    return ::parseNumberSlash(string, number);
}

/************************************************************************/

bool AssetKey::parseNumberSlash(std::string_view& string, uint32_t& number)
{
    return ::parseNumberSlash(string, number);
}

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
        parseNumberSlash(string, appId) &&
        parseNumberSlash(string, classId))
    {
        parseNumberSlash(string, instanceId);
        return true;
    }
    return false;
}

/************************************************************************/

boost::json::value AssetKey::toJson() const
{
    boost::json::object json;
    if (appId) json["appId"]=appId;
    if (classId) json["classId"]=classId;
    if (instanceId) json["instanceId"]=instanceId;
    return json;
}