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

#include "./Helpers.hpp"
#include "Vector.hpp"

/************************************************************************/

CLI::CLI(ConsoleUI& ui_)
    : ui(ui_),
      helpers(std::make_unique<Helpers>(*this))
{
    SteamBot::Startup::InitBase<CLICommandBase, CLI&>::initAll([this](std::unique_ptr<CLICommandBase> command){
        commands.emplace_back(std::move(command));
    }, *this);
}

/************************************************************************/

CLI::~CLI() =default;

/************************************************************************/

CLI::CLICommandBase::~CLICommandBase() =default;

/************************************************************************/

void CLI::CLICommandBase::printSyntax() const
{
    if (needsAccount)
    {
        std::cout << "[<account>:] ";
    }
    std::cout << command;
    if (syntax.size()!=0)
    {
        std::cout << " " << syntax;
    }
}

/************************************************************************/

void CLI::showHelp()
{
    std::cout << "valid commands:" << std::endl;
    for (const auto& command : commands)
    {
        std::cout << "   ";
        command->printSyntax();
        std::cout << " -> " << command->description << std::endl;
    }
}

/************************************************************************/
/*
 * Returns a list of account names to use.
 *
 * Expands things like @groupname or *, or just copies the name.
 */

std::vector<SteamBot::ClientInfo*> expandAccountName(std::string_view name)
{
    std::vector<SteamBot::ClientInfo*> result;

    if (name.starts_with('@'))
    {
        name.remove_prefix(1);
        result=SteamBot::ClientInfo::getGroup(name);
        if (result.empty())
        {
            std::cout << "group \"" << name << "\" not found" << std::endl;
        }
    }
    else if (name=="*")
    {
        result=SteamBot::ClientInfo::getClients();
        SteamBot::erase(result, [](const SteamBot::ClientInfo* info) {
            return !info->getClient();
        });
        if (result.empty())
        {
            std::cout << "no running accounts found" << std::endl;
        }
    }
    else
    {
        if (auto info=SteamBot::ClientInfo::find(name))
        {
            result.push_back(info);
        }
        else
        {
            std::cout << "unknown account \"" << name << "\"" << std::endl;
        }
    }

    return result;
}

/************************************************************************/

static bool executeCommand(SteamBot::ClientInfo* clientInfo, SteamBot::UI::CLI::CLICommandBase* command, std::vector<std::string>& words)
{
    if (!command->execute(clientInfo, words))
    {
        std::cout << "command syntax: ";
        command->printSyntax();
        std::cout << std::endl;
        return false;
    }
    return true;
}

/************************************************************************/
/*
 * Note: commands can be oprefixed with "<accountname>:" as the first
 * word (even if it doesn't make sense, like "account: help").
 */

void CLI::command(std::string_view line)
{
    std::vector<SteamBot::ClientInfo*> clients;

    auto words=getWords(line);

    if (words.size()>0)
    {
        if (words[0].size()>0 && words[0].back()==':')
        {
            auto name=std::move(words[0]);
            name.pop_back();
            words.erase(words.begin());

            clients=expandAccountName(name);
            if (clients.empty())
            {
                return;
            }
        }

        if (clients.empty())
        {
            if (currentAccount!=nullptr)
            {
                clients.push_back(currentAccount);
            }
        }
    }

    if (words.size()>0)
    {
        for (const auto& command : commands)
        {
            if (command->command==words[0])
            {
                if (command->needsAccount)
                {
                    if (clients.empty())
                    {
                        std::cout << "no current account; select one first or specify an account name" << std::endl;
                        return;
                    }

                    bool first=true;
                    for (SteamBot::ClientInfo* clientInfo : clients)
                    {
                        if (!first)
                        {
                            boost::this_fiber::sleep_for(std::chrono::seconds(2));
                        }
                        first=false;

                        if (!executeCommand(clientInfo, command.get(), words))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    executeCommand(nullptr, command.get(), words);
                }
                return;
            }
        }
        std::cout << "unknown command: \"" << words[0] << "\"" << std::endl;
        showHelp();
    }
}

/************************************************************************/

void CLI::run()
{
    typedef SteamBot::UI::ConsoleUI::ManagerBase ManagerBase;

    ui.manager->setMode(ManagerBase::Mode::LineInput);
    {
        std::cout << "Command line mode is now active." << std::endl;
        std::cout << "End it by entering an empty line." << std::endl;
        while (!quit)
        {
            if (currentAccount!=nullptr)
            {
                std::cout << "[" << currentAccount->accountName << "] ";
            }
            std::cout << "command> " << std::flush;
            std::string line;
            std::getline(std::cin, line);
            if (line.empty())
            {
                break;
            }
            command(line);
        }
        std::cout << "Command line mode ended." << std::endl;
    }
    if (quit)
    {
        SteamBot::UI::Thread::quit();
    }
    else
    {
        ui.manager->setMode(ManagerBase::Mode::NoInput);
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useCommonCommands()
{
    useHelpCommand();
    useExitCommand();
    useStatusCommand();
    useSelectCommand();
    useLaunchCommand();
    useCreateCommand();
    useQuitCommand();
}
