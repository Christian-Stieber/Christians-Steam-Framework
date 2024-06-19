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

#include "Client/ClientInfo.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Login
        {
            namespace Internal
            {
                class CredentialsSession
                {
                private:
                    static inline boost::fibers::mutex mutex;
                    static inline std::vector<std::shared_ptr<CredentialsSession>> sessions;

                public:
                    SteamBot::ClientInfo& clientInfo;
                    std::string password;
                    std::string steamGuardCode;

                public:
                    CredentialsSession(SteamBot::ClientInfo&);
                    ~CredentialsSession();

                public:
                    void run_();

                public:
                    static std::shared_ptr<CredentialsSession> get(SteamBot::ClientInfo&);
                    static void remove(std::shared_ptr<CredentialsSession>);
                    static void run(std::shared_ptr<CredentialsSession>);
                };
            }
        }
    }
}
