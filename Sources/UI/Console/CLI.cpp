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
    { "EXIT", "EXIT", &CLI::command_exit },
    { "help", "help", &CLI::command_help }
};

/************************************************************************/

CLI::CLI(ConsoleUI& ui_) : ui(ui_) { }
CLI::~CLI() =default;

/************************************************************************/

bool CLI::command_exit(std::vector<std::string_view>& words)
{
    if (words.size()>1)
    {
        return false;
    }
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

void CLI::showHelp()
{
    std::cout << "valid commands:" << std::endl;
    for (const Command& command : commands)
    {
        std::cout << "   " << command.syntax << std::endl;
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
