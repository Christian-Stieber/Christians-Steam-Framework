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

#include "Asio/Connections.hpp"
#include "Asio/Asio.hpp"
#include "Connection/TCP.hpp"
#include "WebAPI/ISteamDirectory/GetCMList.hpp"
#include "Vector.hpp"
#include "Random.hpp"

#include <boost/fiber/operations.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef SteamBot::Connections Connections;

/************************************************************************/

Connections::Connections() =default;

/************************************************************************/

Connections::Connection::Connection(std::shared_ptr<SteamBot::Client> client_)
    : client(std::move(client_)),
      connection(std::make_unique<SteamBot::Connection::TCP>())
{
}

/************************************************************************/

Connections::Connection::~Connection() =default;

/************************************************************************/

Connections& Connections::get()
{
    static Connections& instance=*new Connections;
    return instance;
}

/************************************************************************/

void Connections::makeConnection(Connections::ConnectResult result, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList> cmList)
{
    // Select a random endpoint
    const size_t index=SteamBot::Random::generateRandomNumber()%cmList->serverlist.size();
    SteamBot::Connection::Endpoint endpoint(cmList->serverlist[index]);
    BOOST_LOG_TRIVIAL(info) << "connecting to " << endpoint.address;

    // ... and try to connect to it
    result->setResult()->connection.connect(endpoint);
    BOOST_LOG_TRIVIAL(info) << "connected to " << endpoint.address;
}

/************************************************************************/

void Connections::getCMList_completed(Connections::ConnectResult result, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList> cmList)
{
    // I'm using the old fiber-based connection code
    boost::fibers::fiber([cmList=std::move(cmList), result=std::move(result)]() mutable {
        static constexpr int maxTries=10;

        int count=0;
        while (true)
        {
            try
            {
                makeConnection(result, cmList);

                // success!
                result->completed();
            }
            catch(...)
            {
                BOOST_LOG_TRIVIAL(error) << "unable to connect to Steam: " << boost::current_exception_diagnostic_information();
                count++;
                if (count==maxTries)
                {
                    throw;
                }
                else
                {
                    boost::this_fiber::sleep_for(std::chrono::seconds(10));
                }
            }
        }
    });
}

/************************************************************************/

void Connections::connect(Connections::ConnectResult result)
{
    // Make sure the client doesn't have a connection already
    SteamBot::erase(connections, [result](std::weak_ptr<Connection>& connection) {
        if (auto locked=connection.lock())
        {
            if (auto connectionClient=locked->client.lock())
            {
                if (auto resultClient=result->setResult()->client.lock())
                {
                    assert(resultClient!=connectionClient);
                    return false;
                }
            }
        }
        return true;
    });

    // Get the Steam endpoints
    SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0, std::bind_front(&Connections::getCMList_completed, std::move(result)));
}

/************************************************************************/
/*
 * This can be called from any thread. It fetches endpoints
 * from Steam, and makes a connection.
 */

Connections::ConnectResult Connections::connect(std::shared_ptr<SteamBot::WaiterBase> waiter, std::shared_ptr<Client> client)
{
    auto result=waiter->createWaiter<ConnectResult::element_type>();
    result->setResult()=std::make_shared<Connection>(std::move(client));

    SteamBot::Asio::post("Connections::connect", [result]() mutable {
        get().connect(std::move(result));
    });

    return result;
}
