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
