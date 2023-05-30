#pragma once

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageServer
        {
            class ServiceMethodMessageBase;
            template <typename REQUEST, typename RESPONSE> class ServiceMethodMessage;
        }
    }
}

/************************************************************************/

class SteamBot::Modules::UnifiedMessageServer::ServiceMethodMessageBase : public SteamBot::Connection::Message::Base
{
public:
    typedef SteamBot::Connection::Message::Header::ProtoBuf headerType;

public:
    static constexpr auto messageType=SteamBot::Connection::Message::Type::ServiceMethod;

    headerType header;
    std::any content;	// std::shared_ptr<REQUEST>

protected:
    ServiceMethodMessageBase(std::span<const std::byte> bytes)
        : header(SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes))
    {
    }

public:
    virtual ~ServiceMethodMessageBase() =default;
};

/************************************************************************/

template <typename REQUEST, typename RESPONSE=void> class SteamBot::Modules::UnifiedMessageServer::ServiceMethodMessage
    : public SteamBot::Modules::UnifiedMessageServer::ServiceMethodMessageBase
{
    static_assert(std::is_same_v<RESPONSE, void>);	// for now...

public:
    ServiceMethodMessage(std::span<const std::byte> bytes)
        : ServiceMethodMessageBase(bytes)
    {
        deserialize(bytes);
    }

    virtual ~ServiceMethodMessage() =default;

public:
    virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
    {
        header.deserialize(deserializer);
        // ToDo: verify that the job-target string is as expected?

        auto contentPtr=std::make_shared<REQUEST>
        
        // ToDo: deserialize the content
    }

    using Serializeable::deserialize;

public:
    static void createdWaiter()
    {
        SteamBot::Modules::Connection::Internal::Handler<ServiceMethodMessage>::create();
    }
}

/************************************************************************/





public:
    std::any content;	// std::shared_ptr<RESPONSE>




    virtual size_t serialize(SteamBot::Connection::Serializer&) const override
    {
        assert(false);
        return 0;
    }
