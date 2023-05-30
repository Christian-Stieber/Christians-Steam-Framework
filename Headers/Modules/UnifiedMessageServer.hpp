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
