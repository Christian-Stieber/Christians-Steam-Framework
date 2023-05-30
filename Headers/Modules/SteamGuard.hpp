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

#include <string>

/************************************************************************/
/*
 * Call registerAccount() to register the current account for needing
 * a SteamGuard code.
 *
 * Then, after the client restart, the module will set the
 * SteamGuardCode in the whiteboard. This is a dual-use item: the
 * connection module will wait for it to appear before actually making
 * the connection, and the Login module will use it as well.
 *
 * If the account wasn't flagged for SteamGuard, it will simply be
 * an empty string. Otherwise, the user will be asked to enter the
 * code and it will be set in the whiteboard.
 */

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace SteamGuard
        {
            void registerAccount();
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace SteamGuard
        {
            namespace Whiteboard
            {
                class SteamGuardCode : public std::string
                {
                };
            }
        }
    }
}
