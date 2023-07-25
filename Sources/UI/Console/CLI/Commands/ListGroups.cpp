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

#include "Helpers/JSON.hpp"

/************************************************************************/

namespace
{
    class ListGroupsCommand : public CLI::CLICommandBase
    {
    public:
        ListGroupsCommand(CLI& cli_)
            : CLICommandBase(cli_, "list-groups", "", "list groups", false)
        {
        }

        virtual ~ListGroupsCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    ListGroupsCommand::InitClass<ListGroupsCommand> init1;
}

/************************************************************************/

bool ListGroupsCommand::execute(SteamBot::ClientInfo*, std::vector<std::string>& words)
{
    if (words.size()!=1)
    {
        return false;
    }

    std::unordered_map<std::string, std::vector<const SteamBot::ClientInfo*>> groups;

    auto clients=SteamBot::ClientInfo::getClients();
    for (const SteamBot::ClientInfo* info : clients)
    {
        auto& dataFile=SteamBot::DataFile::get(info->accountName, SteamBot::DataFile::FileType::Account);
        dataFile.examine([info, &groups](const boost::json::value& json) {
            if (auto array=SteamBot::JSON::getItem(json, "Groups"))
            {
                for (const auto& group : array->as_array())
                {
                    groups[group.as_string().subview()].push_back(info);
                }
            }
        });
    }

    for (const auto& group : groups)
    {
        std::cout << "@" << group.first << ":";
        for (const SteamBot::ClientInfo* info : group.second)
        {
            std::cout << " " << info->accountName;
        }
        std::cout << std::endl;
    }

    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useListGroupsCommand()
{
}
