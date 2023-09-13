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

#include "Client.hpp"
#include "Startup.hpp"

/************************************************************************/
/*
 * A client module derives from Client::Module.
 *
 * In your source file, construct a GLOBAL instance of
 *    Client::Module::Init<T>
 * Clients will use this to find and instanciate your module.
 *
 * Note: init() will be called after all modules have been
 * constructed; if you need other modules in your init, use
 * this instead of doing it in the constructor.
 *
 * Client::Module:
 *   - each module has a "waiter" instance (with a cancellation
 *     object attached to it)
 *   - "run()" is called inside a fiber
 */

/************************************************************************/

namespace SteamBot
{
    template<typename T> concept ClientModule=std::is_base_of_v<Client::Module, T>;

    class Client::Module : public std::enable_shared_from_this<Client::Module>
    {
    public:
        template <typename T> using Init=SteamBot::Startup::Init<Module, T>;

    public:
        const std::shared_ptr<SteamBot::Waiter> waiter;

    protected:
        Module();

        void waitForLogin()
        {
            Client::waitForLogin();
        }

    public:
        virtual ~Module();
        static void createAll(std::function<void(std::shared_ptr<Client::Module>)>);

    public:
        static SteamBot::Client& getClient() { return SteamBot::Client::getClient(); }

        virtual void init(Client&);
        virtual void run(Client&);
    };
}
