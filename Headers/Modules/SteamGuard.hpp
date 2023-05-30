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
