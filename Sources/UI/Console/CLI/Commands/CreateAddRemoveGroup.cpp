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
    enum ChangeMode { Create, Add, Remove };

    static bool checkGroup(const std::vector<SteamBot::ClientInfo*>& clients, ChangeMode mode)
    {
        switch(mode)
        {
        case ChangeMode::Create:
            return clients.empty();

        case ChangeMode::Add:
        case ChangeMode::Remove:
            return !clients.empty();
        }

        assert(false);
        return false;
    }

    static bool changeGroup(std::vector<std::string>& words, ChangeMode mode)
    {
        if (words.size()>=3)
        {
            std::string_view groupName=words[1];
            if (groupName.starts_with('@'))
            {
                groupName.remove_prefix(1);
            }
            auto clients=SteamBot::ClientInfo::getGroup(groupName);
            if (checkGroup(clients, mode))
            {
                for (size_t i=2; i<words.size(); i++)
                {
                    auto info=SteamBot::ClientInfo::find(words[i]);
                    if (info!=nullptr)
                    {
                        auto& dataFile=SteamBot::DataFile::get(info->accountName, SteamBot::DataFile::FileType::Account);
                        dataFile.update([mode, groupName](boost::json::value& json) {
                            auto& array=SteamBot::JSON::createItem(json, "Groups");
                            if (array.is_null())
                            {
                                array.emplace_array();
                            }
                            for (auto iterator=array.as_array().begin(); iterator!=array.as_array().end(); ++iterator)
                            {
                                if (iterator->as_string()==groupName)
                                {
                                    switch(mode)
                                    {
                                    case ChangeMode::Create:
                                    case ChangeMode::Add:
                                        return false;

                                    case ChangeMode::Remove:
                                        array.as_array().erase(iterator);
                                        if (array.as_array().empty())
                                        {
                                            SteamBot::JSON::eraseItem(json, "Groups");
                                        }
                                        return true;
                                    }
                                }
                            }
                            switch(mode)
                            {
                            case ChangeMode::Create:
                            case ChangeMode::Add:
                                array.as_array().emplace_back(groupName);
                                return true;

                            case ChangeMode::Remove:
                                return false;
                            }

                            assert(false);
                            return false;
                        });
                    }
                    else
                    {
                        std::cout << "unknown account \"" << words[i] << "\"" << std::endl;
                    }
                }
            }
            else
            {
                switch(mode)
                {
                case ChangeMode::Create:
                    std::cout << "cannot create group \"" << groupName << "\": group already exists" << std::endl;
                    break;

                case ChangeMode::Add:
                    std::cout << "cannot add to group \"" << groupName << "\": no such group" << std::endl;
                    break;

                case ChangeMode::Remove:
                    std::cout << "cannot remove from group \"" << groupName << "\": no such group" << std::endl;
                    break;
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }
}

/************************************************************************/

namespace
{
    class CreateGroupCommand : public CLI::CLICommandBase
    {
    public:
        CreateGroupCommand(CLI& cli_)
            : CLICommandBase(cli_, "create-group", "[@]<group> <account>...", "create a new group", false)
        {
        }

        virtual ~CreateGroupCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>& words) override
        {
            return changeGroup(words, ChangeMode::Create);
        }
    };

    CreateGroupCommand::InitClass<CreateGroupCommand> init1;
}

/************************************************************************/

namespace
{
    class AddGroupCommand : public CLI::CLICommandBase
    {
    public:
        AddGroupCommand(CLI& cli_)
            : CLICommandBase(cli_, "add-group", "[@]<group> <account>...", "add to a group", false)
        {
        }

        virtual ~AddGroupCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>& words) override
        {
            return changeGroup(words, ChangeMode::Add);
        }
    };

    AddGroupCommand::InitClass<AddGroupCommand> init2;
}

/************************************************************************/

namespace
{
    class RemoveGroupCommand : public CLI::CLICommandBase
    {
    public:
        RemoveGroupCommand(CLI& cli_)
            : CLICommandBase(cli_, "remove-group", "[@]<group> <account>...", "remove from a group", false)
        {
        }

        virtual ~RemoveGroupCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>& words) override
        {
            return changeGroup(words, ChangeMode::Remove);
        }
    };

    RemoveGroupCommand::InitClass<RemoveGroupCommand> init3;
}

/************************************************************************/

void SteamBot::UI::CLI::useCreateAddRemoveGroupCommands()
{
}
