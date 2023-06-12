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

#pragma once

#include "UI/UI.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace UI
    {
        class ConsoleUI : public SteamBot::UI::Base
        {
        public:
            class ManagerBase;
            class Manager;
            class CLI;

        private:
            std::unique_ptr<ManagerBase> manager;
            std::unique_ptr<CLI> cli;

        public:
            ConsoleUI();
            virtual ~ConsoleUI();

        private:
            void performCli();

        private:
            virtual void outputText(ClientInfo&, std::string) override;
            virtual void requestPassword(ClientInfo&, ResultParam<std::string>, PasswordType, bool(*)(const std::string&)) override;
        };
    }
}

/************************************************************************/
/*
 * This runs on an additional thread, and does two things:
 *
 * - manage the "terminal mode"
 * - watch for the "command key"
 *
 * Note: this is only implemented for Linux right now.
 */

class SteamBot::UI::ConsoleUI::ManagerBase
{
protected:
    ConsoleUI& ui;

public:
    enum Mode {
        None,		/* internal use */
        Startup,	/* internal use */
        Shutdown,	/* internal use */
        LineInput,
        LineInputNoEcho,
        NoInput,
    };

    virtual void setMode(Mode) =0;

protected:
    ManagerBase(ConsoleUI&);

public:
    static std::unique_ptr<ManagerBase> create(ConsoleUI&);
    virtual ~ManagerBase();
};

/************************************************************************/

class SteamBot::UI::ConsoleUI::CLI
{
private:
    class Command
    {
    public:
        std::string_view command;
        std::string_view syntax;
        bool(CLI::*function)(std::vector<std::string_view>&);
    };

    static const Command commands[];

private:
    bool quit=false;

private:
    void showHelp();
    void command(std::string_view);

public:
    /* Return 'false' from commands to show the syntax line */
    bool command_exit(std::vector<std::string_view>&);
    bool command_help(std::vector<std::string_view>&);

private:
    ConsoleUI& ui;

public:
    CLI(ConsoleUI&);
    ~CLI();

public:
    void run();
};
