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

#include <memory>
#include <type_traits>

/************************************************************************/
/*
 * A client module derives from Client::Module.
 *
 * In your source file, construct a GLOBAL instance of
 *    Client::ModuleBase::Init<T>
 * Clients will use this to find and instanciate your module.
 *
 * Client::ModuleBase:
 *   - each module has a "waiter" instance (with a cancellation
 *     object attached to it)
 *   - "run()" is called inside a fiber
 *
 * Client::Module;
 *   - "run()" will be called after the login
 */

/************************************************************************/

namespace SteamBot
{
    template<typename T> concept ClientModule=std::is_base_of_v<Client::ModuleBase, T>;

    class Client::ModuleBase : public std::enable_shared_from_this<Client::ModuleBase>
    {
    public:
        class InitBase;
        template <ClientModule T> class Init;

    public:
        const std::shared_ptr<SteamBot::Waiter> waiter;

    protected:
        ModuleBase();

    public:
        virtual ~ModuleBase();
        virtual void invoke(Client&);		// internal use
        static void createAll(std::function<void(std::shared_ptr<Client::ModuleBase>)>);

    public:
        static SteamBot::Client& getClient() { return SteamBot::Client::getClient(); }

        virtual void run(Client&);
    };
}

/************************************************************************/

class SteamBot::Client::ModuleBase::InitBase
{
public:
    const InitBase* const next;

protected:
    InitBase();
    virtual ~InitBase();

public:
    virtual std::shared_ptr<Client::ModuleBase> create() const =0;
};

/************************************************************************/

template <SteamBot::ClientModule T> class SteamBot::Client::ModuleBase::Init : public SteamBot::Client::ModuleBase::InitBase
{
public:
    Init() =default;
    virtual ~Init() =default;

    virtual std::shared_ptr<Client::ModuleBase> create() const override
    {
        return std::make_shared<T>();
    }
};

/************************************************************************/

namespace SteamBot
{
    class Client::Module : public Client::ModuleBase
    {
    protected:
        Module();

    public:
        virtual ~Module();
        virtual void invoke(Client&) override;
    };
}
