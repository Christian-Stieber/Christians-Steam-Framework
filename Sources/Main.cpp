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
    SteamBot::ClientInfo::init();

    SteamBot::UI::Thread::outputText("Welcome to Christian's work-in-progress SteamBot");
    SteamBot::UI::Thread::outputText("Note: use the TAB or RETURN key to enter command mode");
    SteamBot::UI::Thread::wait();

    BOOST_LOG_TRIVIAL(debug) << "exiting";
	return EXIT_SUCCESS;
}
