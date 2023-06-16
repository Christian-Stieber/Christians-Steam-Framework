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

#include "Client/Client.hpp"
#include "Client/ResultWaiter.hpp"
#include "Connection/Encrypted.hpp"

#include <memory>

/************************************************************************/

namespace SteamBot
{
    namespace WebAPI
    {
        namespace ISteamDirectory
        {
            class GetCMList;
        }
    }
}

/************************************************************************/
/*
 * This amanages all client connections.
 * It runs on the Asio thread.
 */

namespace SteamBot
{
    class Connections
    {
    public:
        class Connection
        {
            friend class Connections;

        private:
            std::weak_ptr<Client> client;
            SteamBot::Connection::Encrypted connection;

        public:
            Connection(std::shared_ptr<Client>);	// internal use
            ~Connection();
        };

        std::vector<std::weak_ptr<Connection>> connections;

    public:
        typedef std::shared_ptr<SteamBot::ResultWaiter<std::shared_ptr<Connection>>> ConnectResult;

    private:
        Connections();
        ~Connections() =delete;

        static Connections& get();

        static void makeConnection(ConnectResult, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList>);
        static void getCMList_completed(ConnectResult, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList>);
        void connect(Connections::ConnectResult);

    public:
        static ConnectResult connect(std::shared_ptr<SteamBot::WaiterBase>, std::shared_ptr<Client>);
    };
}
