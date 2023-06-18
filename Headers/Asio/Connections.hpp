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
 * This manages all client connections.
 * It runs on the Asio thread.
 */

namespace SteamBot
{
    class Connections
    {
    public:
        class Connection;

    public:
        std::vector<std::weak_ptr<Connection>> connections;

    public:
        typedef std::shared_ptr<Connection> ConnectResult;

    private:
        Connections();
        ~Connections() =delete;

        static Connections& get();

        static bool readPacket(ConnectResult::weak_type);
        static void run(ConnectResult::weak_type);
        static void makeConnection(ConnectResult, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList>);
        static void getCMList_completed(ConnectResult, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList>);
        void connect(Connections::ConnectResult);

    public:
        static ConnectResult connect(std::shared_ptr<SteamBot::WaiterBase>, std::shared_ptr<Client>);
    };
}

/************************************************************************/
/*
 * A connection is also a waiter-item. It becomes active when one
 * of the following is true:
 *  - a packet is available for reading
 *  - the connection status has changed
 *
 * Also, you can call writePacket() from other threads.
 */

class SteamBot::Connections::Connection : public SteamBot::Waiter::ItemBase
{
    friend class Connections;

public:
    enum Status { Connecting, Connected, GotEOF, Error };

private:
    // This is owned by the asio-thread
    std::weak_ptr<Client> client;
    std::shared_ptr<SteamBot::Connection::Encrypted> connection;

private:
    // Get the mutex before using this
    mutable boost::fibers::mutex mutex;
    Status status=Status::Connecting;
    std::queue<std::vector<std::byte>> readPackets;
    bool statusChanged=true;

private:
    void setStatus(Status);

private:
    virtual bool isWoken() const override;

public:
    Connection(std::shared_ptr<SteamBot::WaiterBase>&& waiter, std::shared_ptr<Client>);	// internal use
    virtual ~Connection();

public:
    Status peekStatus() const;
    Status getStatus();						// this will reset the changed status
    std::vector<std::byte> readPacket();	// empty when there's none

    void writePacket(std::vector<std::byte>);
};