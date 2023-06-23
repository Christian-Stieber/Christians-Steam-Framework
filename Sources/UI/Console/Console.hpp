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
#include "Modules/OwnedGames.hpp"
#include "Modules/LicenseList.hpp"

/************************************************************************/

namespace SteamBot
{
    class Client;
    class ClientInfo;
}

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
        std::string_view description;
        bool(CLI::*function)(std::vector<std::string>&);

    public:
        void printSyntax() const;
    };

    static const Command commands[];

private:
    SteamBot::ClientInfo* currentAccount=nullptr;
    bool quit=false;

private:
    static bool parseNumber(std::string_view, uint64_t&);

    SteamBot::ClientInfo* getAccount() const;
    SteamBot::ClientInfo* getAccount(std::string_view) const;
    void showHelp();
    void command(std::string_view);

    static std::vector<std::string> getWords(std::string_view);

public:
    static SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames::Ptr getOwnedGames(const SteamBot::ClientInfo&);
    static std::vector<std::shared_ptr<const SteamBot::Modules::LicenseList::Whiteboard::Licenses::LicenseInfo>> getLicenseInfo(const SteamBot::ClientInfo&, SteamBot::AppID);

private:
    bool simpleCommand(std::vector<std::string>&, std::function<bool(std::shared_ptr<SteamBot::Client>, uint64_t)>);
    bool game_start_stop(std::vector<std::string>&, bool);

public:
    /* Return 'false' from commands to show the syntax line */
    bool command_exit(std::vector<std::string>&);
    bool command_help(std::vector<std::string>&);
    bool command_status(std::vector<std::string>&);
    bool command_launch(std::vector<std::string>&);
    bool command_create(std::vector<std::string>&);
    bool command_select(std::vector<std::string>&);
    bool command_list_games(std::vector<std::string>&);
    bool command_play_game(std::vector<std::string>&);
    bool command_stop_game(std::vector<std::string>&);
    bool command_add_license(std::vector<std::string>&);

private:
    ConsoleUI& ui;

public:
    CLI(ConsoleUI&);
    ~CLI();

public:
    void run();
};
