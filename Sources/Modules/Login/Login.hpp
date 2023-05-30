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

#include "Steam/OSType.hpp"
#include "Universe.hpp"
#include "Connection/Endpoint.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver_2.hpp"

#include <string>
#include <boost/optional.hpp>

/************************************************************************/
/*
 * SteamKit2, SteamUser.cs, LogOnDetails
 * Note: I pretty much "borrowed" their descriptions :-)
 *
 * ********** cellId
 *  ???
 * 
 * ********** loginID
 *
 * This number is used for identifying logon session.
 *
 * The purpose of this field is to allow multiple sessions to the
 * same steam account from the same machine.  This is because
 * Steam Network doesn't allow more than one session with the same
 * LoginID to access given account at the same time from the same
 * public IP.  If you want to establish more than one active
 * session to given account, you must make sure that every session
 * (to that account) from the same public IP has a unique LoginID.
 *
 * By default LoginID is automatically generated based on
 * machine's primary bind address, which is the same for all
 * sessions.  Null value will cause this property to be
 * automatically generated based on default behaviour.  If in
 * doubt, set this property to null.
 *
 * ********** loginKey
 * ********** shouldRememberPassword
 *
 * This controls password-less login. LoginKey is received from
 * a previous session through the "LoginKeyCallback"
 *
 * ********** sentryFileHash
 * The SHA1 hash of the sentry file received from SteamGuard
 *
 * ********** accountID
 *
 * The account ID used for connecting clients when using the
 * Console instance.
 *
 * ********** requestSteam2Ticket
 *
 * Whether or not to request a Steam2 ticket.
 * This is an optional request only needed for Steam2 content downloads.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace Login
        {
            namespace Internal
            {
                class LoginDetails
                {
                public:
                    // SteamKit2, SteamID.cs, AllInstances/DesktopInstance/ConsoleInstance/WebInstance
                    enum class AccountInstanceType {
                        All=0,
                        Desktop=1,
                        Console=2,
                        Web=4
                    };

                public:
                    boost::optional<std::string> username;
                    boost::optional<std::string> password;

                    boost::optional<uint32_t> cellId;

                    boost::optional<uint32_t> loginId;	// IPv4 only

                    boost::optional<std::string> steamGuardAuthCode;
                    boost::optional<std::string> app2FactorCode;

                    boost::optional<std::string> loginKey;
                    bool shouldRememberPassword=false;

                    std::string sentryFileHash;	// or "empty"

                    AccountInstanceType accountInstance=AccountInstanceType::Desktop;

                    uint32_t accountID=0;
                    bool requestSteam2Ticket=false;

                    Steam::OSType clientOSType=Steam::getOSType();
                    std::string clientLanguage="english";
                };
            }
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Login
        {
            namespace Internal
            {
                void fillClientLogonMessage(Steam::CMsgClientLogonMessageType&,
                                            const SteamBot::Modules::Login::Internal::LoginDetails&,
                                            const SteamBot::Connection::Endpoint);

                void handleCmsgClientUpdateMachineAuth(std::shared_ptr<const Steam::CMsgClientUpdateMachineAuthMessageType>);
                std::string getSentryHash();
            }
        }
    }
}
