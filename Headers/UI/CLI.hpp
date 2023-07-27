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

#include "Startup.hpp"

#include <string_view>

/************************************************************************/

namespace SteamBot
{
    class ClientInfo;

    namespace UI
    {
        class ConsoleUI;
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace UI
    {
        class CLI
        {
        public:
            class CLICommandBase;
            class Helpers;

        private:
            ConsoleUI& ui;
            std::vector<std::unique_ptr<CLICommandBase>> commands;

        public:
            std::unique_ptr<Helpers> helpers;
            SteamBot::ClientInfo* currentAccount=nullptr;
            bool quit=false;

        public:
            CLI(ConsoleUI&);
            ~CLI();

        public:
            SteamBot::ClientInfo* getAccount() const;
            SteamBot::ClientInfo* getAccount(std::string_view) const;
            void showHelp();
            void command(std::string_view);

            static std::vector<std::string> getWords(std::string_view);

        public:
            void run();

        public:
            static void useCommonCommands();

            // added by useCommonCommands()
            static void useHelpCommand();
            static void useExitCommand();
            static void useStatusCommand();
            static void useLaunchCommand();
            static void useCreateCommand();
            static void useSelectCommand();
            static void useQuitCommand();

            // you must add these individually
            static void useListGamesCommand();
            static void usePlayStopGameCommands();
            static void useAddLicenseCommand();
            static void useClearQueueCommand();
            static void useSettingsCommand();
            static void useListInventoryCommand();
            static void useSendInventoryCommand();
            static void useAcceptTradeCommand();
            static void useDeclineTradeCommand();
            static void useCancelTradeCommand();
            static void useListTradeOffersCommand();
            static void useCreateAddRemoveGroupCommands();
            static void useListGroupsCommand();

            static void useSaleEventCommand();
            static void useSaleStickerCommand();
            static void useSaleQueueCommand();

        };
    }
}

/************************************************************************/

class SteamBot::UI::CLI::CLICommandBase
{
public:
    template <typename T> using InitClass=SteamBot::Startup::Init<CLICommandBase, T, CLI&>;

public:
    CLI& cli;

    const std::string_view command;
    const std::string_view syntax;
    const std::string_view description;

    const bool needsAccount=false;

protected:
    template <typename COMMAND, typename SYNTAX, typename DESCRIPTION> CLICommandBase(CLI& cli_, COMMAND&& command_, SYNTAX&& syntax_, DESCRIPTION&& description_, bool needsAccount_)
        : cli(cli_),
          command(std::forward<COMMAND>(command_)),
          syntax(std::forward<SYNTAX>(syntax_)),
          description(std::forward<DESCRIPTION>(description_)),
          needsAccount(needsAccount_)
    {
    }

public:
    virtual ~CLICommandBase();

public:
    virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) =0;
    void printSyntax() const;
};
