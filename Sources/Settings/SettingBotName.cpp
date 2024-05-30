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

/************************************************************************/

typedef SteamBot::Settings::SettingBotName SettingBotName;

/************************************************************************/

bool SettingBotName::setString(std::string_view string)
{
    if (auto info=SteamBot::ClientInfo::find(string))
    {
        if (&(SteamBot::Client::getClient().getClientInfo())!=info)
        {
            clientInfo=info;
            return true;
        }
    }
    return false;
}

/************************************************************************/

std::string_view SettingBotName::getString() const
{
    if (clientInfo!=nullptr)
    {
        return clientInfo->accountName;
    }
    return std::string_view();
}
