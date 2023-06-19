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

#include "Client/Client.hpp"
#include "./Console.hpp"
#include "Modules/Executor.hpp"

#include <charconv>

/************************************************************************/

typedef SteamBot::UI::ConsoleUI::CLI CLI;

/************************************************************************/

const CLI::Command CLI::commands[]={
    { "EXIT", "", "Exit the bot", &CLI::command_exit },
    { "help", "", "Show list of commands", &CLI::command_help },
    { "status", "", "Show list of known accounts", &CLI::command_status },
    { "launch", "[<account>]", "launch an existing bot", &CLI::command_launch },
    { "create", "<account>", "create a new bot", &CLI::command_create },
    { "select", "<account>", "select a bot as target for some commands", &CLI::command_select },
    { "list-games", "[<account>] [<regex>]", "list owned games", &CLI::command_list_games },
    { "play-game", "[<account>] <appid>", "play a game", &CLI::command_play_game },
    { "stop-game", "[<account>] <appid>", "stop playing", &CLI::command_stop_game }
};

/************************************************************************/

CLI::CLI(ConsoleUI& ui_) : ui(ui_) { }
CLI::~CLI() =default;

/************************************************************************/

bool CLI::command_exit(std::vector<std::string>& words)
{
    if (words.size()>1) return false;

    quit=true;
    return true;
}

/************************************************************************/

bool CLI::command_help(std::vector<std::string>&)
{
    showHelp();
    return true;
}

/************************************************************************/

bool CLI::command_status(std::vector<std::string>& words)
{
    if (words.size()>1) return false;

    for (auto client: SteamBot::ClientInfo::getClients())
    {
        std::cout << "   " << client->accountName << std::endl;
    }

    return true;
}

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

bool CLI::command_select(std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        if (currentAccount==nullptr)
        {
            return false;
        }
        currentAccount=nullptr;
        std::cout << "deselected the current account" << std::endl;
    }
    else if (words.size()==2)
    {
        const auto clientInfo=getAccount(words[1]);
        if (clientInfo!=nullptr)
        {
            currentAccount=clientInfo;
            std::cout << "your current account is now \"" << currentAccount->accountName << "\"" << std::endl;
        }
    }
    else
    {
        return false;
    }
    return true;
}

/************************************************************************/

bool CLI::command_launch(std::vector<std::string>& words)
{
    SteamBot::ClientInfo* clientInfo=nullptr;

    if (words.size()==1)
    {
        clientInfo=getAccount();
    }
    else if (words.size()==2)
    {
        clientInfo=getAccount(words[1]);
    }
    else
    {
        return false;
    }

    if (clientInfo!=nullptr)
    {
        SteamBot::Client::launch(*clientInfo);
        std::cout << "launched client \"" << clientInfo->accountName << "\"" << std::endl;
        std::cout << "NOTE: leave command mode to be able to see password/SteamGuard prompts!" << std::endl;
    }
    return true;
}

/************************************************************************/

bool CLI::command_create(std::vector<std::string>& words)
{
    if (words.size()!=2) return false;

    const auto clientInfo=SteamBot::ClientInfo::create(std::string(words[1]));
    if (clientInfo==nullptr)
    {
        std::cout << "account \"" << words[1] << "\" already exists" << std::endl;
    }
    else
    {
        SteamBot::Client::launch(*clientInfo);
        std::cout << "launched new client \"" << clientInfo->accountName << "\"" << std::endl;
        std::cout << "NOTE: leave command mode to be able to see password/SteamGuard prompts!" << std::endl;
    }
    return true;
}

/************************************************************************/

void CLI::Command::printSyntax() const
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
    for (const Command& command : commands)
    {
        std::cout << "   ";
        command.printSyntax();
        std::cout << " -> " << command.description << std::endl;
    }
}

/************************************************************************/

void CLI::command(std::string_view line)
{
    auto words=getWords(line);
    if (words.size()>0)
    {
        for (const Command& command : commands)
        {
            if (command.command==words[0])
            {
                if (!(this->*(command.function))(words))
                {
                    std::cout << "command syntax: ";
                    command.printSyntax();
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

SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames::Ptr CLI::getOwnedGames(SteamBot::ClientInfo& clientInfo)
{
    SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames::Ptr ownedGames;
    if (auto client=clientInfo.getClient())
    {
        SteamBot::Modules::Executor::execute(std::move(client), [&ownedGames](SteamBot::Client& client) mutable {
            if (auto games=client.whiteboard.has<decltype(ownedGames)>())
            {
                ownedGames=*games;
            }
        });
    }
    return ownedGames;
}

/************************************************************************/

bool CLI::parseNumber(std::string_view string, uint64_t& value)
{
    const auto last=string.data()+string.size();
    const auto result=std::from_chars(string.data(), last, value);
    if (result.ec==std::errc() && result.ptr==last)
    {
        return true;
    }
    std::cout << "\"" << string << "\" is not a valid number" << std::endl;
    return false;
}

/************************************************************************/

void CLI::run()
{
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
