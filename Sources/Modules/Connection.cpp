#include "Client/Module.hpp"
#include "Connection/TCP.hpp"
#include "Connection/Encrypted.hpp"
#include "Connection/Message.hpp"
#include "Modules/Connection.hpp"
#include "Modules/SteamGuard.hpp"
#include "Modules/Connection_Internal.hpp"
#include "WebAPI/ISteamDirectory/GetCMList.hpp"
#include "Random.hpp"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/trivial.hpp>
#include <boost/fiber/barrier.hpp>

#include <unordered_map>
#include <typeinfo>

#include "Steam/ProtoBuf/steammessages_base.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

/************************************************************************/

typedef SteamBot::WebAPI::ISteamDirectory::GetCMList GetCMList;
typedef SteamBot::Connection::Endpoint Endpoint;
typedef SteamBot::Modules::Connection::Internal::HandlerBase HandlerBase;
typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;

/************************************************************************/

namespace
{
    class ReadPacketErrorMessage
    {
    public:
        std::exception_ptr exception;

    public:
        ReadPacketErrorMessage()
            : exception(std::current_exception())
        {
        }
    };
}

/************************************************************************/

namespace
{
    class ConnectionStatusSetter
    {
    public:
        void operator()(ConnectionStatus status, SteamBot::Connection::Base* connection=nullptr)
        {
            assert((status==ConnectionStatus::Connected)==(connection!=nullptr));

            auto& whiteboard=SteamBot::Client::getClient().whiteboard;

            if (connection!=nullptr)
            {
                SteamBot::Modules::Connection::Whiteboard::LocalEndpoint endpoint;
                connection->getLocalAddress(endpoint);
                whiteboard.set(std::move(endpoint));
            }
            else
            {
                whiteboard.clear<SteamBot::Modules::Connection::Whiteboard::LocalEndpoint>();
            }

            whiteboard.set(status);
        }

    public:
        ConnectionStatusSetter()
        {
            operator()(ConnectionStatus::Disconnected);
        }

        ~ConnectionStatusSetter()
        {
            operator()(ConnectionStatus::Disconnected);
        }
    };
}

/************************************************************************/

HandlerBase::HandlerBase() =default;
HandlerBase::~HandlerBase() =default;

/************************************************************************/

namespace
{
    class ConnectionModule : public SteamBot::Client::Module
    {
    private:
        typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;

    private:
        SteamBot::Connection::Encrypted connection;

        std::unordered_map<SteamBot::Connection::Message::Type, std::unique_ptr<HandlerBase>> handlers;

    private:
        void connect();
        void readPackets();

    public:
        ConnectionModule();
        virtual ~ConnectionModule();

        virtual void run() override;

    public:
        void add(SteamBot::Connection::Message::Type type, std::unique_ptr<HandlerBase>&& handler)
        {
            bool success=handlers.try_emplace(type, std::move(handler)).second;
            if (success)
            {
                BOOST_LOG_TRIVIAL(info) << "added handler for " << SteamBot::enumToStringAlways(type) << " messages";
            }
        }

    public:
        void handlePacket(std::span<const std::byte>) const;
    };

    ConnectionModule::Init<ConnectionModule> init;
}

/************************************************************************/

void HandlerBase::add(SteamBot::Connection::Message::Type type, std::unique_ptr<HandlerBase>&& handler)
{
    SteamBot::Client::getClient().getModule<ConnectionModule>().add(type, std::move(handler));
}

/************************************************************************/
/*
 * Select a random endpoint from the list
 */

static Endpoint getEndpoint(const std::shared_ptr<const GetCMList>& cmList)
{
    const size_t index=SteamBot::Random::generateRandomNumber()%cmList->serverlist.size();
    auto& endpoint=cmList->serverlist[index];
    BOOST_LOG_TRIVIAL(info) << "client connecting to " << endpoint;
    return endpoint;
}

/************************************************************************/

ConnectionModule::ConnectionModule()
    : connection(std::make_unique<SteamBot::Connection::TCP>())
{
    BOOST_LOG_TRIVIAL(debug) << "ConnectionModule::ConnectionModule " << this;
}

/************************************************************************/

ConnectionModule::~ConnectionModule()
{
    BOOST_LOG_TRIVIAL(debug) << "ConnectionModule::~ConnectionModule " << this;
}

/************************************************************************/

void ConnectionModule::handlePacket(std::span<const std::byte> bytes) const
{
    const auto messageType=SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes);
    auto iterator=handlers.find(messageType);
    if (iterator!=handlers.end())
    {
        BOOST_LOG_TRIVIAL(info) << "received message type " << SteamBot::enumToStringAlways(messageType);
        auto handler=iterator->second.get();
        handler->handle(bytes);
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "ignoring message type " << SteamBot::enumToStringAlways(messageType);
    }
}

/************************************************************************/

void ConnectionModule::readPackets()
{
    getClient().launchFiber("ConnectionModule::readPackets", [this](){
        try
        {
            while (true)
            {
                const auto bytes=connection.readPacket();
                handlePacket(bytes);
            }
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(info) << "ConnectionModule::readPackets got an exception";
            getClient().messageboard.send(std::make_shared<ReadPacketErrorMessage>());
        }
    });
}

/************************************************************************/

void ConnectionModule::run()
{
    getClient().launchFiber("ConnectionModule::run", [this](){
        ConnectionStatusSetter connectionStatus;

        SteamBot::Waiter waiter;

        {
            typedef SteamBot::Modules::SteamGuard::Whiteboard::SteamGuardCode SteamGuardCode;
            auto steamGuardWaiter=waiter.createWaiter<SteamBot::Whiteboard::Waiter<SteamGuardCode>>(getClient().whiteboard);
            do
            {
                waiter.wait();
            }
            while (!steamGuardWaiter->has());
        }

        connectionStatus(ConnectionStatus::Connecting);
        connect();
        connectionStatus(ConnectionStatus::Connected, &connection);
        readPackets();

        auto readPacketErrorWaiter=waiter.createWaiter<SteamBot::Messageboard::Waiter<ReadPacketErrorMessage>>(getClient().messageboard);
        auto sendMessageWaiter=waiter.createWaiter<SteamBot::Messageboard::Waiter<SendSteamMessage>>(getClient().messageboard);
        try
        {
            while (true)
            {
                waiter.wait();

                {
                    auto message=readPacketErrorWaiter->fetch();
                    if (message)
                    {
                        std::rethrow_exception(message->exception);
                    }
                }

                {
                    std::shared_ptr<const SendSteamMessage> message;
                    while ((message=sendMessageWaiter->fetch()))
                    {
                        message->payload->send(connection);
                        typedef SteamBot::Modules::Connection::Whiteboard::LastMessageSent LastMessageSent;
                        SteamBot::Client::getClient().whiteboard.set(LastMessageSent(std::chrono::steady_clock::now()));
                    }
                }
            }
        }
        catch(const boost::system::system_error& exception)
        {
            if (exception.code()==boost::asio::error::eof)
            {
                BOOST_LOG_TRIVIAL(info) << "got EOF from socket";
                SteamBot::Client::getClient().whiteboard.set(SteamBot::Client::RestartClient());
            }
            throw;
        }
    });
}

/************************************************************************/

void ConnectionModule::connect()
{
    // ToDo: what's the cellId used for?
    auto getCMList=SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0);

    static constexpr int maxTries=10;

    int count=0;
    while (true)
    {
        try
        {
            auto endpoint=getEndpoint(getCMList);
            connection.connect(endpoint);
			BOOST_LOG_TRIVIAL(info) << "client connected";
            return;
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
                boost::this_fiber::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

/************************************************************************/

typedef SteamBot::Modules::Connection::Messageboard::SendSteamMessage SendSteamMessage;

/************************************************************************/

SendSteamMessage::SendSteamMessage(SendSteamMessage::MessageType payload_)
    : payload(std::move(payload_))
{
}

/************************************************************************/

SendSteamMessage::~SendSteamMessage() =default;

/************************************************************************/

void SendSteamMessage::send(SendSteamMessage::MessageType payload)
{
    auto message=std::make_shared<SendSteamMessage>(std::move(payload));
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}

/************************************************************************/

void SteamBot::Modules::Connection::handlePacket(std::span<const std::byte> bytes)
{
    SteamBot::Client::getClient().getModule<ConnectionModule>().handlePacket(bytes);
}

/************************************************************************/

static void waitMessageDestruct(std::shared_ptr<SteamBot::DestructMonitor>& message)
{
    class Callback : public SteamBot::DestructMonitor::DestructCallback
    {
    public:
        virtual ~Callback() =default;

    private:
        boost::fibers::barrier barrier{2};

    private:
        virtual void call(const SteamBot::DestructMonitor*) override
        {
            barrier.wait();
        }

    public:
        void wait()
        {
            barrier.wait();
        }
    };

    auto callback=std::make_shared<Callback>();
    message->destructCallback=callback;
    message.reset();
    callback->wait();
}

/************************************************************************/
/*
 * This deserializes the packet, and posts it to the messageboard.
 *
 * For specific message types, it will wait until the message is
 * destructed (i.. all recipients have received and processed
 * the message). This ensures that
 *   - for CMsgMulti, the messages are not interlaved with
 *     later messages from the network connection
 *   - for CMsgClientLogonResponse, the message is fully
 *     processed before a potential connection close
 *     is detected and the client is shut down
 */

void HandlerBase::handle(SteamBot::Connection::Base::ConstBytes bytes) const
{
    auto message=send(bytes);
    const auto& messageType=typeid(*message);
    if (messageType==typeid(Steam::CMsgMultiMessageType) ||
        messageType==typeid(Steam::CMsgClientLogonResponseMessageType))
    {
        waitMessageDestruct(message);
    }
}
