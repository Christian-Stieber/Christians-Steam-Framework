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
#include "Modules/PlayGames.hpp"

/************************************************************************/

typedef SteamBot::Modules::PlayGames::Messageboard::PlayGame PlayGame;
typedef SteamBot::UI::ConsoleUI::CLI CLI;

/************************************************************************/

bool CLI::game_start_stop(std::vector<std::string>& words, bool start)
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
        uint64_t appId;
        if (parseNumber(words.back(), appId))
        {
            bool success=false;
            if (auto client=clientInfo->getClient())
            {
                success=SteamBot::Modules::Executor::execute(std::move(client), [appId, start](SteamBot::Client&) mutable {
                    PlayGame::play(static_cast<SteamBot::AppID>(appId), start);
                });

                if (success)
                {
                    std::cout << (start ? "started" : "stopped") << " game " << appId;
                    if (auto ownedGames=getOwnedGames(*clientInfo))
                    {
                        if (auto info=ownedGames->getInfo(static_cast<SteamBot::AppID>(appId)))
                        {
                            std::cout << " (" << info->name << ")";
                        }
                    }
                    std::cout << " on account " << clientInfo->accountName << std::endl;
                }
            }
            if (!success)
            {
                std::cout << "unable to launch game " << appId << " on account " << clientInfo->accountName << std::endl;
            }
        }
    }

    return true;
}

/************************************************************************/

bool CLI::command_play_game(std::vector<std::string>& words)
{
    return game_start_stop(words, true);
}

/************************************************************************/

bool SteamBot::UI::ConsoleUI::CLI::command_stop_game(std::vector<std::string>& words)
{
    return game_start_stop(words, false);
}
