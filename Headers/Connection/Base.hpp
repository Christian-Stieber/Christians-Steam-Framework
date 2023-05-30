#pragma once

#include "Connection/Endpoint.hpp"

#include <span>

/************************************************************************/
/*
 * This is an abstract base that acts as an interface; it's bascially
 * the IConnection of SteamKit2.
 */

namespace SteamBot
{
    namespace Connection
    {
        class Base
        {
        public:
            typedef std::span<std::byte> MutableBytes;
            typedef std::span<const std::byte> ConstBytes;
            
        protected:
            Base();

        public:
            virtual ~Base();

        public:
            virtual void connect(const Endpoint&) =0;
            virtual void disconnect() =0;
            virtual MutableBytes readPacket() =0;
            virtual void writePacket(ConstBytes) =0;

            virtual void getLocalAddress(Endpoint&) const =0;

        public:
            virtual void cancel() =0;
        };
    }
}

/************************************************************************/
/*
 * This is the base class for all Connection-related exceptions that
 * want to complain about something that was received from Steam.
 */

namespace SteamBot
{
    namespace Connection
    {
        class DataException { };
    }
}
