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

#include "Asio/Signals.hpp"
#include "WorkingDir.hpp"
#include "Client/ClientInfo.hpp"
#include "Logging.hpp"
#include "Main.hpp"

#include <locale>

/************************************************************************/

int main(void)
{
	std::locale::global(std::locale::classic());
	SteamBot::setWorkingDir();

    SteamBot::Logging::init();
    SteamBot::ClientInfo::init();

    SteamBot::initSignals();

    application();

    BOOST_LOG_TRIVIAL(debug) << "exiting";
	return EXIT_SUCCESS;
}
