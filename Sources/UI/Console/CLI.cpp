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

/************************************************************************/

typedef SteamBot::UI::ConsoleUI::CLI CLI;

/************************************************************************/
/*
 * A simple tokenizer that just looks for whitespace-separated chunks
 * of characters.
 *
 * For now(?), we don't even need quoted strings.
 */

static std::vector<std::string_view> getWords(std::string_view line)
{
    std::vector<std::string_view> result;

    class Word
    {
    private:
        const char* start;
        const char* end;

    public:
        void clear()
        {
            start=nullptr;
            end=nullptr;
        }

        Word()
        {
            clear();
        }

        operator std::string_view() const
        {
            assert(start!=nullptr);
            return std::string_view(start, end+1);
        }

        void append(const char* c)
        {
            if (start==nullptr)
            {
                start=c;
            }
            else
            {
                assert(c==end+1);
            }
            end=c;
        }

        bool empty() const
        {
            return start==nullptr;
        }
    } word;

    const char* const end=line.data()+line.size();
    for (const char* c=line.data(); c!=end; c++)
    {
        if (*c==' ' || *c=='\n' || *c=='\t')
        {
            if (!word.empty())
            {
                result.emplace_back(word);
                word.clear();
            }
        }
        else
        {
            word.append(c);
        }
    }

    if (!word.empty())
    {
        result.emplace_back(word);
        word.clear();
    }

    return result;
}

/************************************************************************/

const CLI::Command CLI::commands[]={
    { "EXIT", "EXIT", "Exit the bot", &CLI::command_exit },
    { "help", "help", "Show list of commands", &CLI::command_help },
    { "status", "status", "Show list of known accounts", &CLI::command_status },
    { "launch", "launch [<account>]", "launch an existing bot", &CLI::command_launch },
    { "create", "create <account>", "create a new bot", &CLI::command_create },
    { "select", "select <account>", "select a bot as target for some commands", &CLI::command_select }
};

/************************************************************************/

CLI::CLI(ConsoleUI& ui_) : ui(ui_) { }
CLI::~CLI() =default;

/************************************************************************/

bool CLI::command_exit(std::vector<std::string_view>& words)
{
    if (words.size()>1) return false;

    quit=true;
    return true;
}

/************************************************************************/

bool CLI::command_help(std::vector<std::string_view>&)
{
    showHelp();
    return true;
}

/************************************************************************/

bool CLI::command_status(std::vector<std::string_view>& words)
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

bool CLI::command_select(std::vector<std::string_view>& words)
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

bool CLI::command_launch(std::vector<std::string_view>& words)
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

bool CLI::command_create(std::vector<std::string_view>& words)
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

void CLI::showHelp()
{
    std::cout << "valid commands:" << std::endl;
    for (const Command& command : commands)
    {
        std::cout << "   " << command.syntax << " -> " << command.description << std::endl;
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
                    std::cout << "command syntax: " << command.syntax << std::endl;
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
