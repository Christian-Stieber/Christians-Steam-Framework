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
#include "Modules/Executor.hpp"
#include "Modules/PlayGames.hpp"
#include "Modules/Login.hpp"
#include "Modules/OwnedGames.hpp"

/************************************************************************/

namespace
{
    class StatusCommand : public CLI::CLICommandBase
    {
    public:
        StatusCommand(CLI& cli_)
            : CLICommandBase(cli_, "status", "", "Show list of known accounts", false)
        {
        }

        virtual ~StatusCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    StatusCommand::InitClass<StatusCommand> init;
}

/************************************************************************/

bool StatusCommand::execute(SteamBot::ClientInfo*, std::vector<std::string>& words)
{
    if (words.size()>1) return false;

    for (auto clientInfo: SteamBot::ClientInfo::getClients())
    {
        std::ostringstream output;
        output << "   " << clientInfo->accountName;
        if (auto client=clientInfo->getClient())
        {
            SteamBot::Modules::Executor::execute(std::move(client), [&output](SteamBot::Client& client) mutable {
                typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
                typedef SteamBot::Modules::PlayGames::Whiteboard::PlayingGames PlayingGames;
                typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
                switch (client.whiteboard.get<LoginStatus>(LoginStatus::LoggedOut))
                {
                case LoginStatus::LoggedOut:
                    break;

                case LoginStatus::LoggingIn:
                    output << "; logging in";
                    break;

                case LoginStatus::LoggedIn:
                    if (auto playing=client.whiteboard.has<PlayingGames>())
                    {
                        assert(!playing->empty());

                        auto ownedGames=client.whiteboard.has<OwnedGames::Ptr>();
                        const char* separator="; playing ";
                        for (SteamBot::AppID appId : *playing)
                        {
                            output << separator << static_cast<std::underlying_type_t<decltype(appId)>>(appId);
                            if (ownedGames)
                            {
                                if (auto info=(*ownedGames)->getInfo(appId))
                                {
                                    output << " (" << info->name << ")";
                                }
                            }
                            separator=", ";
                        }
                    }
                    else
                    {
                        output << "; logged in";
                    }
                    break;
                }
            });
        }
        std::cout << output.view() << std::endl;
    }

    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useStatusCommand()
{
}
