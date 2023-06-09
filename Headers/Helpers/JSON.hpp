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

#include <boost/json.hpp>

/************************************************************************/
/*
 * Get the item at a path (which must be a series of
 * std::string_view).
 *
 * Returns the json value, or a nullptr.
 */

namespace SteamBot
{
    namespace JSON
    {
        template <typename... REST> static const boost::json::value* getItem(const boost::json::value& json, std::string_view first, REST&&... rest)
        {
            if (auto child=json.as_object().if_contains(first))
            {
                if constexpr (sizeof...(REST)>0)
                {
                    return getItem(*child, std::forward<REST>(rest)...);
                }
                return child;
            }
            return nullptr;
        }
    }
}

/************************************************************************/
/*
 * Similar to getItem(), but creates intermediate objects and
 * the item (as a null value) itself.
 */

namespace SteamBot
{
    namespace JSON
    {
        template <typename... REST> static boost::json::value& createItem(boost::json::value& json, std::string_view first, REST&&... rest)
        {
            auto& object=json.as_object();
            auto child=object.if_contains(first);
            if (child==nullptr)
            {
                boost::json::object::iterator iterator;
                if constexpr (sizeof...(REST)>0)
                {
                    iterator=object.emplace(first, boost::json::object()).first;
                }
                else
                {
                    iterator=object.emplace(first, nullptr).first;
                }
                child=&(iterator->value());
            }
            if constexpr (sizeof...(REST)>0)
            {
                return createItem(*child, std::forward<REST>(rest)...);
            }
            return *child;
        }
    }
}

/************************************************************************/
/*
 * Delete the item at a path (which must be a series of
 * std::string_view), as well as intermediate empty objects.
 */

namespace SteamBot
{
    namespace JSON
    {
        template <typename... REST> static void eraseItem(boost::json::value& json, std::string_view first, REST&&... rest)
        {
            auto& object=json.as_object();
            if constexpr (sizeof...(REST)>0)
            {
                if (auto child=object.if_contains(first))
                {
                    eraseItem(*child, std::forward<REST>(rest)...);
                    if (child->as_object().empty())
                    {
                        object.erase(first);
                    }
                }
            }
            else
            {
                object.erase(first);
            }

        }
    }
}
