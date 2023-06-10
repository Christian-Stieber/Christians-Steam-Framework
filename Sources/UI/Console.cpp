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

#include <iostream>
#include <time.h>

/************************************************************************/

namespace
{
    class ConsoleUI : public SteamBot::UI::Base
    {
    public:
        ConsoleUI() =default;
        virtual ~ConsoleUI() =default;

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
        return stream
            << clientInfo.accountName
            << " ["
            << SteamBot::Time::toString(clientInfo.when, false)
            << "]: ";
    }
}

/************************************************************************/

void ConsoleUI::outputText(ClientInfo& clientInfo, std::string text)
{
    std::cout << clientInfo << text << std::flush;
}

/************************************************************************/

void ConsoleUI::requestPassword(ClientInfo& clientInfo, ResultParam<std::string> result, SteamBot::UI::Base::PasswordType passwordType, bool(*validator)(const std::string&))
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

/************************************************************************/

std::unique_ptr<SteamBot::UI::Base> SteamBot::UI::createConsole()
{
    return std::make_unique<ConsoleUI>();
}
