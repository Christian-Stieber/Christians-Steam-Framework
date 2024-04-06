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

#include "Helpers/ParseNumber.hpp"

#include <optional>

#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace URLs
    {
        class NoURLException { };

        boost::urls::url getClientCommunityURL();
    }
}

/************************************************************************/
/*
 * Set a URL param to an integer value. No localization.
 */

namespace SteamBot
{
    namespace URLs
    {
        template <std::integral T> void setParam_(boost::urls::url&, const char*, T);

        template <std::integral T> void setParam(boost::urls::url& url, const char* name, T value)
        {
            if constexpr (std::is_signed_v<T>)
            {
                return setParam_(url, name, static_cast<long long>(value));
            }
            else
            {
                return setParam_(url, name, static_cast<unsigned long long>(value));
            }
        }
    }
}

/************************************************************************/
/*
 * Returns a numeric query param
 */

namespace SteamBot
{
    namespace URLs
    {
        template <typename T> std::optional<T> getParam(const boost::urls::url_view& url, std::string_view name)
        {
            std::optional<T> result;

            const auto& params=url.params();
            auto iterator=params.find_last(name);
            if (iterator!=params.end())
            {
                T value;
                if (SteamBot::parseNumber((*iterator).value, value))
                {
                    result=value;
                }
            }
            return result;
        }
    }
}
