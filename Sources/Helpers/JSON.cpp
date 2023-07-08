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

#include "Helpers/JSON.hpp"

/************************************************************************/

const boost::json::string* SteamBot::JSON::optString(const boost::json::value& json, std::string_view key)
{
    if (auto item=json.as_object().if_contains(key))
    {
        return &(item->as_string());
    }
    return nullptr;
}

/************************************************************************/

bool SteamBot::JSON::optString(const boost::json::value& json, std::string_view key, std::string& result)
{
    if (auto string=optString(json, key))
    {
        result=*string;
        return true;
    }
    return false;
}

/************************************************************************/

bool SteamBot::JSON::optBool(const boost::json::value& json, std::string_view key, bool& result)
{
    if (auto item=json.as_object().if_contains(key))
    {
        if (item->is_number())
        {
            auto value=item->to_number<unsigned int>();
            result=(value!=0);
        }
        else
        {
            result=item->as_bool();
        }
        return true;
    }
    return false;
}
