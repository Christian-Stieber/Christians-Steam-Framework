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

#include "Modules/OwnedGames.hpp"
#include "Helpers/NumberString.hpp"

#include <span>

/************************************************************************/
/*
 * Note: we don't currently(?) have a way to protect the AppInfo data
 * in any way, since we just keep the JSON from the file "as is".
 *
 * That's why the examine() function uses a callback -- it will
 * lock the mutex while you're in the callback.
 */

namespace SteamBot
{
    namespace AppInfo
    {
        void update(const SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames&);
        bool examine(std::function<bool(const boost::json::value&)>);

        std::optional<boost::json::value> get(std::span<const std::string_view>);

        template <typename... ARGS> std::optional<boost::json::value> get(SteamBot::AppID appId, ARGS&& ...args)
        {
            auto appIdString=SteamBot::toString(SteamBot::toInteger(appId));
            std::array<std::string_view, 1+sizeof...(args)> array{appIdString, std::forward<std::string_view>(args)...};
            return get(array);
        }

        std::vector<SteamBot::AppID> getDLCs(SteamBot::AppID);
    }
}
