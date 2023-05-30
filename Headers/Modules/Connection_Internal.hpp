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

#include "Client/Client.hpp"
#include "DestructMonitor.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Connection
        {
            namespace Internal
            {
                class HandlerBase
                {
                protected:
                    HandlerBase();

                public:
                    virtual ~HandlerBase();

                protected:
                    static void add(SteamBot::Connection::Message::Type, std::unique_ptr<HandlerBase>&&);

                private:
                    virtual std::shared_ptr<SteamBot::DestructMonitor> send(SteamBot::Connection::Base::ConstBytes) const =0;

                public:
                    void handle(SteamBot::Connection::Base::ConstBytes) const;
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
        namespace Connection
        {
            namespace Internal
            {
                template <typename T> class Handler : public HandlerBase
                {
                public:
                    Handler() =default;
                    virtual ~Handler() =default;

                private:
                    virtual std::shared_ptr<SteamBot::DestructMonitor> send(SteamBot::Connection::Base::ConstBytes bytes) const override
                    {
                        auto message=std::make_shared<T>(bytes);
                        SteamBot::Client::getClient().messageboard.send(message);
                        return message;
                    }

                public:
                    static void create()
                    {
                        add(T::messageType, std::make_unique<Handler<T>>());
                    }
                };
            }
        }
    }
}
