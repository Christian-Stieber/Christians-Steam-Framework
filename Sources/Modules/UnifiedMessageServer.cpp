#include "Client/Module.hpp"
#include "Connection/Message.hpp"

/************************************************************************/
/*
 * Note: this needs A LOT more work, to let clients register
 * subscriptions for templated versions of the ServiceMethod message
 * class, link these with the job_name etc.
 *
 * For now, this simple version is merely there so we can see
 * what calls we actually get.
 */

/************************************************************************/

namespace
{
    class UnifiedMessageServerModule : public SteamBot::Client::Module
    {
    public:
        UnifiedMessageServerModule() =default;
        virtual ~UnifiedMessageServerModule() =default;

        virtual void run() override;
    };

    UnifiedMessageServerModule::Init<UnifiedMessageServerModule> init;
}

/************************************************************************/

namespace
{
    class ServiceMethodMessage : public SteamBot::Connection::Message::Base
    {
    public:
        typedef SteamBot::Connection::Message::Header::ProtoBuf headerType;

    public:
        static constexpr auto messageType=SteamBot::Connection::Message::Type::ServiceMethod;

        headerType header;
        // no content... yet?

    public:
        ServiceMethodMessage(std::span<const std::byte> bytes)
            : header(SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes))
        {
            deserialize(bytes);
        }

        virtual ~ServiceMethodMessage() =default;

    public:
        virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
        {
            header.deserialize(deserializer);
        }

        virtual size_t serialize(SteamBot::Connection::Serializer&) const override
        {
            assert(false);
            return 0;
        }

        using Serializeable::deserialize;

    public:
        static void createdWaiter()
        {
            SteamBot::Modules::Connection::Internal::Handler<ServiceMethodMessage>::create();
        }
    };
}

/************************************************************************/

void UnifiedMessageServerModule::run()
{
    getClient().launchFiber("HeartbeatModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<ServiceMethodMessage>> serviceMethodMessage;
        serviceMethodMessage=waiter.createWaiter<decltype(serviceMethodMessage)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter.wait();
            serviceMethodMessage->fetch();
        }
    });
}
