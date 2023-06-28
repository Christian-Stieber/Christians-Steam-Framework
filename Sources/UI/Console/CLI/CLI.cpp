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

/************************************************************************/

CLI::CLI(ConsoleUI& ui_)
    : ui(ui_),
      helpers(std::make_unique<Helpers>())
{
    SteamBot::Startup::InitBase<CLICommandBase, CLI&>::initAll([this](std::unique_ptr<CLICommandBase> command){
        commands.emplace_back(std::move(command));
    }, *this);
}

/************************************************************************/

CLI::~CLI() =default;

/************************************************************************/
/*
 * Returns currentAccount, or nullptr.
 *
 * Outputs a message if none was set.
 */

SteamBot::ClientInfo* CLI::getAccount() const
{
    if (currentAccount==nullptr)
    {
        std::cout << "no current account; select one first or specify an account name" << std::endl;
    }
    return currentAccount;
}

/************************************************************************/
/*
 * Returns named account, or nullptr.
 *
 * Outputs a message if none was found.
 */

SteamBot::ClientInfo* CLI::getAccount(std::string_view name) const
{
    const auto clientInfo=SteamBot::ClientInfo::find(name);
    if (clientInfo==nullptr)
    {
        std::cout << "unknown account \"" << name << "\"" << std::endl;
    }
    return clientInfo;
}

/************************************************************************/

CLI::CLICommandBase::~CLICommandBase() =default;

/************************************************************************/

void CLI::CLICommandBase::printSyntax() const
{
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

void CLI::command(std::string_view line)
{
    auto words=getWords(line);
    if (words.size()>0)
    {
        for (const auto& command : commands)
        {
            if (command->command==words[0])
            {
                if (!command->execute(words))
                {
                    std::cout << "command syntax: ";
                    command->printSyntax();
                    std::cout << std::endl;
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
    useStatusCommand();
    useSelectCommand();
    useLaunchCommand();
}
