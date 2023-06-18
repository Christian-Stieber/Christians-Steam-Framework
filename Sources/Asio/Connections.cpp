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

#include "Connection/TCP.hpp"
#include "Asio/Asio.hpp"
#include "Asio/Connections.hpp"
#include "WebAPI/ISteamDirectory/GetCMList.hpp"
#include "Random.hpp"

#include <boost/fiber/operations.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef SteamBot::Connections Connections;
typedef Connections::Connection Connection;

/************************************************************************/

Connections::Connections() =default;

/************************************************************************/

Connection::Connection(std::shared_ptr<SteamBot::WaiterBase>&& waiter)
    : ItemBase(std::move(waiter))
{
    auto baseConnection=std::make_unique<SteamBot::Connection::TCP>();
    connection=std::make_shared<SteamBot::Connection::Encrypted>(std::move(baseConnection));

    BOOST_LOG_TRIVIAL(debug) << "created Steam connection " << this;
}

/************************************************************************/

Connection::~Connection()
{
    assert(!writingPackets);
    auto& myConnection=connection;

    SteamBot::Asio::post("Connections::disconnect", [connection=std::move(myConnection)]() {
        connection->cancel();
    });

    BOOST_LOG_TRIVIAL(debug) << "destroying Steam connection " << this;
}

/************************************************************************/

void Connection::install(std::shared_ptr<ItemBase> base)
{
    auto item=std::dynamic_pointer_cast<Connection>(base);
    assert(item);
    self=item;
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
    bool result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        result=(statusChanged || !readPackets.empty());
    }
    BOOST_LOG_TRIVIAL(debug) << "Connection " << this << " isWoken? -> " << result;
    return result;
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

decltype(Connection::localEndpoint) Connection::getLocalEndpoint() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return localEndpoint;
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

    std::lock_guard<decltype(result->mutex)> lock(result->mutex);
    result->connection->getLocalAddress(result->localEndpoint);
}

/************************************************************************/

static void logException(const boost::system::system_error& exception)
{
    boost::log::trivial::severity_level logLevel;

    switch(exception.code().value())
    {
    case boost::asio::error::eof:
        logLevel=boost::log::trivial::info;
        break;

    case boost::asio::error::operation_aborted:
        logLevel=boost::log::trivial::debug;
        break;

    default:
        logLevel=boost::log::trivial::error;
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
                {
                    std::vector<std::byte> bytes(packet.begin(), packet.end());
                    std::lock_guard<decltype(locked->mutex)> lock(locked->mutex);
                    locked->readPackets.emplace(std::move(bytes));
                }
                locked->wakeup();
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
    assert(SteamBot::Asio::isThread());

    try
    {
        while (readPacket(result))
            ;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "exception on Steam connection: " << boost::current_exception_diagnostic_information();
    }

    BOOST_LOG_TRIVIAL(debug) << "Steam connection is ending the run loop";
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
    }).detach();
}

/************************************************************************/

void Connection::doWritePackets()
{
    assert(SteamBot::Asio::isThread());

    while (peekStatus()==Status::Connected)
    {
        std::vector<std::byte> packet;
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            if (writePackets.empty())
            {
                return;
            }
            packet=std::move(writePackets.front());
            writePackets.pop();
        }

        try
        {
            connection->writePacket(packet);
        }
        catch(const boost::system::system_error& exception)
        {
            logException(exception);

            switch(exception.code().value())
            {
            case boost::asio::error::eof:
                setStatus(Connection::Status::GotEOF);
                break;

            case boost::asio::error::operation_aborted:
                assert(false);
                break;

            default:
                setStatus(Connection::Status::Error);
                break;
            }
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(error) << "exception on Steam connection: " << boost::current_exception_diagnostic_information();
            setStatus(Connection::Status::Error);
        }
    }
}

/************************************************************************/
/*
 * This can be called from any thread. It fetches endpoints
 * from Steam, and makes a connection.
 *
 * Just delete the connection if you don't need it anymore.
 */

Connections::ConnectResult Connections::connect(std::shared_ptr<SteamBot::WaiterBase> waiter)
{
    auto result=waiter->createWaiter<ConnectResult::element_type>();

    SteamBot::Asio::post("Connections::create", [result]() mutable {
        SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0, std::bind_front(&Connections::getCMList_completed, std::move(result)));
    });

    return result;
}

/************************************************************************/
/*
 * Again, this can be called from any thread. Probably the same thread
 * that owns the connection, though.
 *
 * Note that this keeps a shared_ptr on the connection while writing,
 * so unless the connection is severed from the remote end, all data
 * will be written.
 */

void Connection::writePacket(std::vector<std::byte> packet)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        writePackets.emplace(std::move(packet));
    }

    auto locked=self.lock();

    SteamBot::Asio::post("Connections::writePacket", [locked=std::move(locked)]() mutable {
        if (!locked->writingPackets)
        {
            bool empty;
            {
                std::lock_guard<decltype(locked->mutex)> lock(locked->mutex);
                empty=locked->writePackets.empty();
            }
            if (!empty)
            {
                locked->writingPackets=true;
                boost::fibers::fiber([locked=std::move(locked)]() {
                    locked->doWritePackets();
                    locked->writingPackets=false;
                }).detach();
            }
        }
    });
}
