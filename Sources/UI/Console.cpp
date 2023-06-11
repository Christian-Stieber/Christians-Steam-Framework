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

#include "UI/UI.hpp"
#include "Helpers/Time.hpp"
#include "./ConsoleMonitor.hpp"

#include <iostream>
#include <time.h>

/************************************************************************/

namespace
{
    class ConsoleUI : public SteamBot::UI::Base
    {
    private:
        std::unique_ptr<SteamBot::UI::ConsoleMonitorBase> monitor;

    public:
        ConsoleUI();
        virtual ~ConsoleUI() =default;

    private:
        void performCli();

    private:
        virtual void outputText(ClientInfo&, std::string) override;
        virtual void requestPassword(ClientInfo&, ResultParam<std::string>, PasswordType, bool(*)(const std::string&)) override;
    };
}

/************************************************************************/

namespace
{
    std::ostream& operator<<(std::ostream& stream, const SteamBot::UI::Base::ClientInfo& clientInfo)
    {
        if (!clientInfo.accountName.empty())
        {
            stream << clientInfo.accountName << " ";
        }
        return stream
            << '['
            << SteamBot::Time::toString(clientInfo.when, false)
            << "]: ";
    }
}

/************************************************************************/

ConsoleUI::ConsoleUI()
{
    monitor=SteamBot::UI::ConsoleMonitorBase::create([this]() {
        BOOST_LOG_TRIVIAL(debug) << "command key pressed";
        executeOnThread([this]() { performCli(); });
    });
    monitor->enable();
}

/************************************************************************/

void ConsoleUI::performCli()
{
    monitor->disable();
    {
        std::cout << "Command line mode is now active." << std::endl;
        std::cout << "End it by entering an empty line." << std::endl;
        while (true)
        {
            std::cout << "command> " << std::flush;
            std::string command;
            std::getline(std::cin, command);
            if (command.empty())
            {
                break;
            }
            std::cout << "ToDo: actually execute your command: " << command << std::endl;
        }
        std::cout << "Command line mode ended." << std::endl;
    }
    monitor->enable();
}

/************************************************************************/

void ConsoleUI::outputText(ClientInfo& clientInfo, std::string text)
{
    std::cout << clientInfo << text << std::flush;
}

/************************************************************************/

void ConsoleUI::requestPassword(ClientInfo& clientInfo, ResultParam<std::string> result, SteamBot::UI::Base::PasswordType passwordType, bool(*validator)(const std::string&))
{
    monitor->disable();
    {
        const char* passwordTypeString=nullptr;
        switch(passwordType)
        {
        case PasswordType::AccountPassword:
            passwordTypeString="account password";
            break;

        case PasswordType::SteamGuard_EMail:
            passwordTypeString="steamguard-code (email)";
            break;

        case PasswordType::SteamGuard_App:
            passwordTypeString="steamguard-code (mobile app)";
            break;
        }
        assert(passwordTypeString!=nullptr);

        std::string entered;
        do
        {
            std::cout << clientInfo << "Please enter " << passwordTypeString << ": " << std::flush;
            std::cin >> entered;
        }
        while (!(*validator)(entered));

        *(result->getResult())=std::move(entered);
        result->completed();
    }
    monitor->enable();
}

/************************************************************************/

std::unique_ptr<SteamBot::UI::Base> SteamBot::UI::createConsole()
{
    return std::make_unique<ConsoleUI>();
}
