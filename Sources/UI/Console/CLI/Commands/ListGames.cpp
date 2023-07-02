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

#include "UI/CLI.hpp"

#include "../Helpers.hpp"

#include "Client/Client.hpp"
#include "Helpers/StringCompare.hpp"
#include "Helpers/Time.hpp"
#include "Modules/PackageData.hpp"

#include <regex>
#include <iomanip>

/************************************************************************/

typedef CLI::Helpers::OwnedGames OwnedGames;

/************************************************************************/

namespace
{
    class ListGamesCommand : public CLI::CLICommandBase
    {
    public:
        ListGamesCommand(CLI& cli_)
            : CLICommandBase(cli_, "list-games", "[<regex>]", "list owned games", true)
        {
        }

        virtual ~ListGamesCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    ListGamesCommand::InitClass<ListGamesCommand> init;
}

/************************************************************************/

static void print(const SteamBot::Modules::LicenseList::Whiteboard::Licenses::LicenseInfo& license)
{
    auto packageIdValue=static_cast<std::underlying_type_t<decltype(license.packageId)>>(license.packageId);
    std::cout << "pkg " << packageIdValue
              << " purchased " << SteamBot::Time::toString(license.timeCreated, false);
}

/************************************************************************/

static void outputGameList(SteamBot::ClientInfo& clientInfo, const OwnedGames::Ptr::element_type& ownedGames, std::string_view pattern)
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
        std::cout << std::setw(8) << static_cast<std::underlying_type_t<decltype(game->appId)>>(game->appId) << ": " << game->name;
        if (game->lastPlayed!=decltype(game->lastPlayed)())
        {
            std::cout << "; last played " << SteamBot::Time::toString(game->lastPlayed);
        }
        if (game->playtimeForever.count()!=0)
        {
            std::cout << "; playtime " << SteamBot::Time::toString(game->playtimeForever);
        }

        auto licenses=CLI::Helpers::getLicenseInfo(clientInfo, game->appId);
        if (licenses.size()==1)
        {
            std::cout << "; ";
            print(*(licenses.front()));
        }
        else
        {
            for (auto& license : licenses)
            {
                std::cout << "\n          ";
                print(*license);
            }
        }
        std::cout << "\n";
    }
    std::cout << std::flush;
}

/************************************************************************/

bool ListGamesCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    std::string_view pattern;

    if (words.size()==2)
    {
        pattern=words[1];
    }
    else if (words.size()==1)
    {
    }
    else
    {
        return false;
    }

    if (auto ownedGames=CLI::Helpers::getOwnedGames(*clientInfo))
    {
        outputGameList(*clientInfo, *ownedGames, pattern);
    }
    else
    {
        std::cout << "gamelist not available for \"" << clientInfo->accountName << "\"" << std::endl;
    }

    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useListGamesCommand()
{
}
