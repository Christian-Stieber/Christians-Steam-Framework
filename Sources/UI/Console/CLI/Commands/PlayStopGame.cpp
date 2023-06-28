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

#include "Modules/Executor.hpp"
#include "Modules/PlayGames.hpp"

/************************************************************************/

typedef SteamBot::Modules::PlayGames::Messageboard::PlayGame PlayGame;

/************************************************************************/

namespace
{
    class PlayBase : public CLI::CLICommandBase
    {
    protected:
        template <typename... ARGS> PlayBase(ARGS&&... args)
            : CLICommandBase(std::forward<ARGS>(args)...)
        {
        }

        virtual ~PlayBase() =default;

    protected:
        bool game_start_stop(std::vector<std::string>&, bool);
    };
}

/************************************************************************/

bool PlayBase::game_start_stop(std::vector<std::string>& words, bool start)
{
    return cli.helpers->simpleCommand(words, [start](std::shared_ptr<SteamBot::Client> client, uint64_t appId){
        bool success=SteamBot::Modules::Executor::execute(client, [appId, start](SteamBot::Client&) mutable {
            PlayGame::play(static_cast<SteamBot::AppID>(appId), start);
        });
        if (success)
        {
            std::cout << (start ? "started" : "stopped") << " game " << appId;
            if (auto ownedGames=CLI::Helpers::getOwnedGames(client->getClientInfo()))
            {
                if (auto info=ownedGames->getInfo(static_cast<SteamBot::AppID>(appId)))
                {
                    std::cout << " (" << info->name << ")";
                }
            }
            std::cout << " on account " << client->getClientInfo().accountName << std::endl;
        }
        return success;
    });
}

/************************************************************************/

namespace
{
    class PlayGameCommand : public PlayBase
    {
    public:
        PlayGameCommand(CLI& cli_)
            : PlayBase(cli_, "play-game", "[<account>] <appid>", "play a game")
        {
        }

        virtual ~PlayGameCommand() =default;

    public:
        virtual bool execute(std::vector<std::string>& words) override
        {
            return game_start_stop(words, true);
        }
    };

    PlayGameCommand::InitClass<PlayGameCommand> playInit;
}

/************************************************************************/

namespace
{
    class StopGameCommand : public PlayBase
    {
    public:
        StopGameCommand(CLI& cli_)
            : PlayBase(cli_, "stop-game", "[<account>] <appid>", "stop playing")
        {
        }

        virtual ~StopGameCommand() =default;

    public:
        virtual bool execute(std::vector<std::string>& words) override
        {
            return game_start_stop(words, false);
        }
    };

    StopGameCommand::InitClass<StopGameCommand> stopInit;
}

/************************************************************************/

void CLI::usePlayStopGameCommands()
{
}
