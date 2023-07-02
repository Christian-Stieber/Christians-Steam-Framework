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
#include "Settings.hpp"
#include "EnumString.hpp"

#include "../Helpers.hpp"

/************************************************************************/

namespace
{
    class SetCommand : public CLI::CLICommandBase
    {
    public:
        SetCommand(CLI& cli_)
            : CLICommandBase(cli_, "set", "[<name> <value>]", "change a setting for an account", true)
        {
        }

        virtual ~SetCommand() =default;

    private:
        static void setVariable(const SteamBot::ClientInfo*, std::string_view name, std::string_view value);
        static void showVariables(const SteamBot::ClientInfo*);

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    SetCommand::InitClass<SetCommand> init;
}

/************************************************************************/

void SetCommand::showVariables(const SteamBot::ClientInfo* clientInfo)
{
    const auto& settings=SteamBot::ClientSettings::get();

    auto variables=settings.getVariables();
    for (const auto& variable : variables)
    {
        std::cout << "   " << variable.first << " (" << SteamBot::enumToStringAlways(variable.second) << ")";
        if (clientInfo!=nullptr)
        {
            switch(variable.second)
            {
            case SteamBot::ClientSettings::Type::Bool:
                {
                    auto value=settings.getBool(variable.first, clientInfo->accountName);
                    if (value)
                    {
                        std::cout << ": " << (*value ? "on" : "off");
                    }
                }
                break;

            default:
                assert(false);
            }
        }
        std::cout << std::endl;
    }
}

/************************************************************************/

static const struct BoolValues
{
    std::string_view string;
    bool value;
} boolValues[]=
{
    { "on", true },
    { "off", false },

    { "enable", true },
    { "disable", false },

    { "yes", true },
    { "no", false },

    { "true", true },
    { "false", false },

    { "1", true },
    { "0", false }
};

/************************************************************************/

void SetCommand::setVariable(const SteamBot::ClientInfo* clientInfo, std::string_view name, std::string_view value)
{
    const auto& settings=SteamBot::ClientSettings::get();

    auto type=settings.getType(name);
    switch(type)
    {
    case SteamBot::ClientSettings::Type::Bool:
        for (const auto& item : boolValues)
        {
            if (item.string==value)
            {
                settings.setBool(name, clientInfo->accountName, item.value);
                break;
            }
        }
        break;

    default:
        assert(false);
    }
}

/************************************************************************/

bool SetCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==3)
    {
        // set <name> <value>
        setVariable(clientInfo, words[1], words[2]);
    }
    else if (words.size()==2)
    {
        // set <name> -> clear
        SteamBot::ClientSettings::get().clear(clientInfo->accountName, words[1]);
    }
    else if (words.size()==1)
    {
        // set -> show all
        showVariables(clientInfo);
    }
    else
    {
        return false;
    }
    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useSettingsCommand()
{
}
