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

#include "Client/Client.hpp"

/************************************************************************/
/*
 * This is for commands with the format
 *   command [<account>] <number>
 *
 * It parses the command line, then calls your callback.
 *
 * If your callback returns "false", we'll output a generic error
 * message about the client not being available.
 */

bool SteamBot::UI::ConsoleUI::CLI::simpleCommand(std::vector<std::string>& words, std::function<bool(std::shared_ptr<SteamBot::Client>, uint64_t)> callback)
{
    SteamBot::ClientInfo* clientInfo=nullptr;

    if (words.size()==3)
    {
        clientInfo=getAccount(words[1]);
    }
    else if (words.size()==2)
    {
        clientInfo=getAccount();
    }
    else
    {
        return false;
    }

    if (clientInfo!=nullptr)
    {
        uint64_t number;
        if (parseNumber(words.back(), number))
        {
            bool success=false;
            if (auto client=clientInfo->getClient())
            {
                success=callback(std::move(client), number);
            }
            if (!success)
            {
                std::cout << "client \"" << clientInfo->accountName << "\" is unavaialble; please launch it first" << std::endl;
            }
        }
    }

    return true;
}
