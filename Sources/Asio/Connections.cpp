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
#include "Client/Client.hpp"
#include "Helpers/JSON.hpp"

#include <boost/fiber/operations.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/fiber/protected_fixedsize_stack.hpp>

/************************************************************************/

typedef SteamBot::Connections Connections;
typedef Connections::Connection Connection;
typedef SteamBot::Connection::Endpoint Endpoint;

/************************************************************************/
/*
 * The key for the last working endpoint in the account file.
 */

static const char previousEndpointKey[]="previousEndpoint";

/************************************************************************/

Connections::Connections() =default;

/************************************************************************/

Connection::Connection(std::shared_ptr<SteamBot::WaiterBase>&& waiter_)
    : ItemBase(std::move(waiter_))
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
 * Get the previous endpoint for the current client
 */

static std::shared_ptr<Endpoint> getPreviousEndpoint()
{
    auto& dataFile=SteamBot::Client::getClient().dataFile;
    auto string=dataFile.examine([](const boost::json::value& json) {
        return SteamBot::JSON::getItem(json, previousEndpointKey);
    });

    if (string!=nullptr)
    {
        try
        {
            return std::make_shared<Endpoint>(string->as_string());
        }
        catch(...)
        {
        }
    }
    return nullptr;
}

/************************************************************************/

void Connection::storeEndpoint() const
{
    auto& dataFile=SteamBot::Client::getClient().dataFile;

    std::ostringstream string;
    string << remoteEndpoint.address << ":" << remoteEndpoint.port;

    dataFile.update([&string](boost::json::value& json) {
        auto& item=SteamBot::JSON::createItem(json, previousEndpointKey);
        if (auto existing=item.if_string())
        {
            if (*existing==string.view())
            {
                return false;
            }
        }
        item.emplace_string()=string.view();
        return true;
    });
}

/************************************************************************/
/*
 * Tries to make a connection to the endpoint. We must be on a fiber
 * on the Asio thread for this to work.
 */

bool Connections::makeConnection(const Endpoint& endpoint, Connections::ConnectResult& result)
{
    BOOST_LOG_TRIVIAL(info) << "connecting to " << endpoint.address << ":" << endpoint.port;

    try
    {
        {
            std::lock_guard<decltype(result->mutex)> lock(result->mutex);
            result->connection->connect(endpoint);
            result->connection->getLocalAddress(result->localEndpoint);
            result->remoteEndpoint=endpoint;
        }
        result->setStatus(Connection::Status::Connected);
        BOOST_LOG_TRIVIAL(info) << "connected to " << endpoint.address << ":" << endpoint.port;
        return true;
    }
    catch (const boost::system::system_error& exception)
    {
        if (exception.code().value()==boost::asio::error::eof)
        {
            // Not sure what's going on here, but it suddenly seems to happen a *lot*
            BOOST_LOG_TRIVIAL(debug) << "remote end closed the connection?";
            result->connection->disconnect();
            return false;
        }
        throw;
    }
}

/************************************************************************/

void Connections::fetchEndpointsAndMakeConnection(Connections::ConnectResult result)
{
    SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0, [result=std::move(result)](std::shared_ptr<const SteamBot::WebAPI::ISteamDirectory::GetCMList> cmList) mutable {
        boost::fibers::fiber(std::allocator_arg, boost::fibers::protected_fixedsize_stack(), [result=std::move(result), cmList=std::move(cmList)]() mutable {
            int count=100;
            while (--count>0)
            {
                const size_t index=SteamBot::Random::generateRandomNumber()%cmList->serverlist.size();
                Endpoint endpoint(cmList->serverlist[index]);
                if (makeConnection(endpoint, result))
                {
                    run(result);
                    break;
                }
                boost::this_fiber::sleep_for(std::chrono::milliseconds(200));
            }
        }).detach();
    });
}

/************************************************************************/
/*
 * This can be called from any thread.
 *
 * It tries to make a connection to the last endpoint that has
 * worked for this connection; if that doesn't work then it
 * fetches endpoints from Steam and tries them.
 *
 * Just delete the connection if you don't need it anymore.
 */

Connections::ConnectResult Connections::connect(std::shared_ptr<SteamBot::WaiterBase> waiter)
{
    auto result=waiter->createWaiter<ConnectResult::element_type>();
    auto previousEndpoint=getPreviousEndpoint();

    SteamBot::Asio::post("Connections::connect", [result, previousEndpoint=std::move(previousEndpoint)]() mutable {
        if (previousEndpoint)
        {
            // We are still using the old fiber-based connection code
            boost::fibers::fiber(std::allocator_arg, boost::fibers::protected_fixedsize_stack(), [result=std::move(result), previousEndpoint=std::move(previousEndpoint)]() mutable {
                if (makeConnection(*previousEndpoint, result))
                {
                    run(result);
                }
                else
                {
                    fetchEndpointsAndMakeConnection(std::move(result));
                }
            }).detach();
        }
        else
        {
            fetchEndpointsAndMakeConnection(std::move(result));
        }
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
