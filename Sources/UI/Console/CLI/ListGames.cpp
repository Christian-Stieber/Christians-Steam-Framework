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
#include "Modules/OwnedGames.hpp"
#include "Helpers/StringCompare.hpp"

#include <regex>

/************************************************************************/

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

/************************************************************************/

static void outputGameList(const OwnedGames::Ptr::element_type& ownedGames, std::string_view pattern)
{
    typedef std::shared_ptr<const OwnedGames::GameInfo> ItemType;

    std::vector<ItemType> games;
    {
        std::regex regex(pattern.begin(), pattern.end(), std::regex_constants::icase);
        for (const auto& item : ownedGames.games)
        {
            if (std::regex_search(item.second->name, regex))
            {
                games.emplace_back(item.second);
            }
        }
    }

    std::sort(games.begin(), games.end(), [](const ItemType& left, const ItemType& right) -> bool {
        return SteamBot::caseInsensitiveStringCompare_less(left->name, right->name);
    });

    for (const auto& game : games)
    {
        std::cout << std::setw(8) << static_cast<std::underlying_type_t<decltype(game->appId)>>(game->appId) << ": " << game->name << "\n";
    }
    std::cout << std::flush;
}

/************************************************************************/

bool SteamBot::UI::ConsoleUI::CLI::command_list_games(std::vector<std::string>& words)
{
    SteamBot::ClientInfo* clientInfo=nullptr;
    std::string_view pattern;

    if (words.size()==3)
    {
        // list-games accountname pattern
        clientInfo=getAccount(words[1]);
        pattern=words[2];
    }
    else if (words.size()==2)
    {
        // list-games accountname
        // list-games pattern
        clientInfo=SteamBot::ClientInfo::find(words[1]);
        if (clientInfo==nullptr)
        {
            clientInfo=getAccount();
            pattern=words[1];
        }
    }
    else if (words.size()==1)
    {
        clientInfo=getAccount();
    }
    else
    {
        return false;
    }

    if (clientInfo)
    {
        OwnedGames::Ptr ownedGames;
        {
            if (auto client=clientInfo->getClient())
            {
                SteamBot::Modules::Executor::execute(std::move(client), [this, &ownedGames](SteamBot::Client& client) mutable {
                    if (auto games=client.whiteboard.has<decltype(ownedGames)>())
                    {
                        ownedGames=*games;
                    }
                });
            }
        }
        if (ownedGames)
        {
            outputGameList(*ownedGames, pattern);
        }
        else
        {
            std::cout << "gamelist not available for \"" << clientInfo->accountName << "\"" << std::endl;
        }
    }

    return true;
}
