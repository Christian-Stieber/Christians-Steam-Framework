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

#include "../Console.hpp"

#include "Modules/Executor.hpp"
#include "Modules/AddFreeLicense.hpp"

/************************************************************************/

typedef SteamBot::Modules::AddFreeLicense::Messageboard::AddLicense AddLicense;
typedef SteamBot::UI::ConsoleUI::CLI CLI;

/************************************************************************/

bool CLI::command_add_license(std::vector<std::string>& words)
{
    return simpleCommand(words, [](std::shared_ptr<SteamBot::Client> client, uint64_t appId){
        bool success=SteamBot::Modules::Executor::execute(client, [appId](SteamBot::Client&) mutable {
            AddLicense::add(static_cast<SteamBot::AppID>(appId));
        });
        if (success)
        {
            std::cout << "I've asked Steam to add the license " << appId << " to your account" << std::endl;
        }
        return success;
    });
}
