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

#include "Connection/Message.hpp"
#include "Modules/Connection.hpp"
#include "Modules/Login.hpp"
#include "JobID.hpp"

#include <any>
#include <any>
#include <google/protobuf/service.h>
#include <boost/callable_traits/args.hpp>
#include <tuple>
#include <type_traits>

/************************************************************************/
/*
 * execute() is the official API
 *
 * Note that the method name looks like this:
 * "<Service>.<Method>#<Version>", example: "Player.GetGameBadgeLevels#1".
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageClient
        {
            template <typename RESPONSE, SteamBot::Connection::Message::Type TYPE=SteamBot::Connection::Message::Type::ServiceMethodCallFromClient, typename REQUEST>
            std::shared_ptr<RESPONSE> execute(std::string_view, REQUEST&&);
        }
    }
}

/************************************************************************/
/*
 * This is an attempt to get datatypes from the protobuf declarations
 *
 * As an example, if protobuf defines a service "Player", it will
 * generate a
 *    class Player
 *
 * For an RPC function "GetGameBadgeLevels" on that service, this class
 * will have a virtual function like so:
 *    virtual void GetGameBadgeLevels(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
 *                                    const ::CPlayer_GetGameBadgeLevels_Request* request,
 *                                    ::CPlayer_GetGameBadgeLevels_Response* response,
 *                                    ::google::protobuf::Closure* done);
 *
 * Thus, we should be able to translate a service/method name to the
 * request and result types...
 *
 * This defines a template "Info" that can be used like this:
 *    Info<decltype(&::<servicename>::<methodname>)>
  * Example:
 *    Info<decltype(&::Player::GetGameBadgeLevels)>
 *
 * Info has these interesting members:
 *    RequestType (typedef)
 *    ResultType (typedef)
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageClient
        {
            namespace ProtobufService
            {
                template <typename T> struct Info
                {
                    typedef boost::callable_traits::args_t<T> arguments;

                    template <size_t i> struct arg
                    {
                        typedef typename std::tuple_element<i, arguments>::type type;
                    };

                    static_assert(std::tuple_size<arguments>::value==5, "service call parameter count mismatch");
                    static_assert(std::is_same<typename arg<1>::type, ::google::protobuf::RpcController*>::value, "service call parameter type mismatch");
                    static_assert(std::is_same<typename arg<4>::type, ::google::protobuf::Closure*>::value, "service call parameter type mismatch");

                    typedef std::remove_const_t<std::remove_pointer_t<typename arg<2>::type>> RequestType;
                    typedef std::remove_pointer_t<typename arg<3>::type> ResultType;
                };
            }
        }
	}
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace UnifiedMessageClient
        {
            namespace Internal
            {
                class ServiceMethodResponseMessage;
                class UnifiedMessageBase;
                template <typename REQUEST, typename RESPONSE, SteamBot::Connection::Message::Type TYPE> class UnifiedMessage;
            }
        }
    }
}

/************************************************************************/

class SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessageBase
{
public:
    const SteamBot::JobID jobId;

private:
    bool receivedResponse=false;
    virtual std::any deserialize(SteamBot::Connection::Deserializer&) const =0;

protected:
    UnifiedMessageBase();

public:
    virtual ~UnifiedMessageBase();

public:
    static std::any deserialize(const SteamBot::JobID&, SteamBot::Connection::Deserializer&);

protected:
    std::shared_ptr<const ServiceMethodResponseMessage> waitForResponse() const;
};

/************************************************************************/
/*
 * The problem with ServiceMethodResponse message is that the type
 * doesn't tell us what the body data is.
 *
 * Instead, we have to deserialize the header to get a jobid, which we
 * can use to find the job -- which knows the expected response type.
 */

class SteamBot::Modules::UnifiedMessageClient::Internal::ServiceMethodResponseMessage : public SteamBot::Connection::Message::Base
{
public:
    typedef SteamBot::Connection::Message::Header::ProtoBuf headerType;

public:
    static constexpr auto messageType=SteamBot::Connection::Message::Type::ServiceMethodResponse;

    headerType header;
    std::any content;	// std::shared_ptr<RESPONSE>

public:
    ServiceMethodResponseMessage(std::span<const std::byte> bytes)
        : header(SteamBot::Connection::Message::Header::Base::peekMessgeType(bytes))
    {
        deserialize(bytes);
    }

    virtual ~ServiceMethodResponseMessage() =default;

public:
    virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
    {
        header.deserialize(deserializer);
        const SteamBot::JobID jobId(header.proto.jobid_target());
        content=UnifiedMessageBase::deserialize(jobId, deserializer);
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
        SteamBot::Modules::Connection::Internal::Handler<ServiceMethodResponseMessage>::create();
    }
};

/************************************************************************/

template <typename REQUEST, typename RESPONSE, SteamBot::Connection::Message::Type TYPE> class SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessage
    : public SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessageBase
{
public:
    typedef SteamBot::Connection::Message::Message<TYPE, SteamBot::Connection::Message::Header::ProtoBuf, REQUEST> RequestMessageType;

public:
    UnifiedMessage() =default;
    virtual ~UnifiedMessage() =default;

public:
    std::shared_ptr<RESPONSE> execute(std::string_view method, REQUEST&& body)
    {
        {
            auto message=std::make_unique<RequestMessageType>();
            message->content=std::move(body);
            message->header.proto.set_jobid_source(jobId.getValue());
            message->header.proto.set_target_job_name(std::string(method));
            SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
        }

        auto message=waitForResponse();
        return std::any_cast<std::shared_ptr<RESPONSE>>(message->content);
    }

private:
    virtual std::any deserialize(SteamBot::Connection::Deserializer& deserializer) const override
    {
        auto result=std::make_shared<RESPONSE>();
        SteamBot::Connection::Message::ContentSerialization<RESPONSE>::deserialize(deserializer, *result);
        return result;
    }
};

/************************************************************************/

template <typename RESPONSE, SteamBot::Connection::Message::Type TYPE, typename REQUEST>
std::shared_ptr<RESPONSE> SteamBot::Modules::UnifiedMessageClient::execute(std::string_view method, REQUEST&& body)
{
    SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessage<REQUEST,RESPONSE,TYPE> message;
    return message.execute(method, std::move(body));
}
