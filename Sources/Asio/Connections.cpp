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
typedef Connections::Connection Connection;

/************************************************************************/

Connections::Connections() =default;

/************************************************************************/

Connection::Connection(std::shared_ptr<SteamBot::WaiterBase>&& waiter, std::shared_ptr<SteamBot::Client> client_)
    : ItemBase(std::move(waiter)),
      client(std::move(client_))
{
    auto baseConnection=std::make_unique<SteamBot::Connection::TCP>();
    connection=std::make_shared<SteamBot::Connection::Encrypted>(std::move(baseConnection));
}

/************************************************************************/

Connection::~Connection()
{
    auto& myConnection=connection;

    SteamBot::Asio::post("Connections::disconnect", [connection=std::move(myConnection)]() {
        connection->cancel();
    });
}

/************************************************************************/

void Connection::setStatus(Connection::Status newStatus)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        status=newStatus;
        statusChanged=true;
    }
    wakeup();
}

/************************************************************************/

bool Connections::Connection::isWoken() const
{
    return statusChanged || !readPackets.empty();
}

/************************************************************************/

Connection::Status Connection::peekStatus() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return status;
}

/************************************************************************/

Connection::Status Connection::getStatus()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    statusChanged=false;
    return status;
}

/************************************************************************/

std::vector<std::byte> Connection::readPacket()
{
    std::vector<std::byte> result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (!readPackets.empty())
        {
            result=std::move(readPackets.front());
            readPackets.pop();
        }
    }
    return result;
}

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
    result->connection->connect(endpoint);
    BOOST_LOG_TRIVIAL(info) << "connected to " << endpoint.address;
}

/************************************************************************/

static void logException(const boost::system::system_error& exception)
{
    boost::log::trivial::severity_level logLevel;

    switch(exception.code().value())
    {
    case boost::asio::error::eof:
        logLevel==boost::log::trivial::info;
        break;

    case boost::asio::error::operation_aborted:
        logLevel==boost::log::trivial::debug;
        break;

    default:
        logLevel==boost::log::trivial::error;
        break;
    }

    BOOST_LOG_STREAM_WITH_PARAMS(boost::log::trivial::logger::get(), (boost::log::keywords::severity=logLevel))
        << "exception on Steam connection: " << boost::current_exception_diagnostic_information();
}

/************************************************************************/
/*
 * Returns true if "all went well" -- i.e. keep going.
 * Returns false if we need to stop using the connection.
 */

bool Connections::readPacket(ConnectResult::weak_type result)
{
    auto locked=result.lock();
    if (locked && locked->peekStatus()==Connection::Status::Connected)
    {
        auto connection=locked->connection;
        assert(connection);

        locked.reset();

        try
        {
            auto packet=connection->readPacket();

            locked=result.lock();
            if (locked)
            {
                std::vector<std::byte> bytes(packet.begin(), packet.end());
                std::lock_guard<decltype(locked->mutex)> lock(locked->mutex);
                locked->readPackets.emplace(std::move(bytes));
                return true;
            }
        }
        catch(const boost::system::system_error& exception)
        {
            logException(exception);

            switch(exception.code().value())
            {
            case boost::asio::error::eof:
                locked=result.lock();
                if (locked)
                {
                    locked->setStatus(Connection::Status::GotEOF);
                }
                break;

            case boost::asio::error::operation_aborted:
                assert(result.expired());
                break;

            default:
                break;
            }
        }
    }
    return false;
}

/************************************************************************/

void Connections::run(ConnectResult::weak_type result)
{
    try
    {
        while (readPacket(result))
            ;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "exception on Steam connection: " << boost::current_exception_diagnostic_information();
    }
}

/************************************************************************/

void Connections::getCMList_completed(Connections::ConnectResult result, std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList> cmList)
{
    // I'm using the old fiber-based connection code
    boost::fibers::fiber([cmList=std::move(cmList), result=std::move(result)]() mutable {
        static constexpr int maxTries=10;

        int count=0;
        while (result->peekStatus()==Connection::Status::Connecting)
        {
            try
            {
                makeConnection(result, cmList);
                result->setStatus(Connection::Status::Connected);
            }
            catch(const boost::system::system_error& exception)
            {
                logException(exception);

                switch(exception.code().value())
                {
                case boost::asio::error::eof:
                    result->setStatus(Connection::Status::GotEOF);
                    break;

                case boost::asio::error::operation_aborted:
                    assert(false);
                    break;

                default:
                    count++;
                    if (count==maxTries)
                    {
                        result->setStatus(Connection::Status::Error);
                    }
                    else
                    {
                        boost::this_fiber::sleep_for(std::chrono::seconds(10));
                    }
                    break;
                }
            }
            catch(...)
            {
                BOOST_LOG_TRIVIAL(error) << "exception on Steam connection: " << boost::current_exception_diagnostic_information();
            }
        }
        run(result);
    });
}

/************************************************************************/

void Connections::connect(Connections::ConnectResult result)
{
    auto client=result->client.lock();

    // Make sure the client doesn't have a connection already
    SteamBot::erase(connections, [result, client=std::move(client)](std::weak_ptr<Connection>& connection) {
        if (auto connectionLocked=connection.lock())
        {
            if (auto connectionClient=connectionLocked->client.lock())
            {
                if (client)
                {
                    assert(client!=connectionClient);
                }
                return false;
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
    auto result=waiter->createWaiter<ConnectResult::element_type>(std::move(client));

    SteamBot::Asio::post("Connections::connect", [result]() mutable {
        get().connect(std::move(result));
    });

    return result;
}
