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

#include "WorkingDir.hpp"
#include "Client/Client.hpp"
#include "UI/UI.hpp"
#include "Logging.hpp"

#include <locale>

/************************************************************************/

std::unique_ptr<SteamBot::UI::Base> SteamBot::UI::create()
{
    return createConsole();
}

/************************************************************************/

int main(void)
{
	std::locale::global(std::locale::classic());
	SteamBot::setWorkingDir();

    SteamBot::Logging::init();

    SteamBot::UI::Thread::outputText("Welcome to Christian's work-in-progress SteamBot");

#if 0
    SteamBot::Client::launchAll();
#else
    SteamBot::UI::Thread::outputText("Test");

    std::shared_ptr<SteamBot::Waiter> waiter=SteamBot::Waiter::create();
    auto passwordWaiter=SteamBot::UI::Thread::requestPassword(waiter, SteamBot::UI::Base::PasswordType::AccountPassword);

    waiter->wait();
    if (auto password=passwordWaiter->getResult())
    {
        SteamBot::UI::Thread::outputText(std::move(*password));
    }
#endif

#if 0
    // do this in a future non-interactive node?
    SteamBot::Client::waitAll();
#else
    SteamBot::UI::Thread::wait();
#endif

    // ToDo: make clients clean up?

    BOOST_LOG_TRIVIAL(debug) << "exiting";
	return EXIT_SUCCESS;
}
