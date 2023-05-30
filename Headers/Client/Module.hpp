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
 *    Client::Module::Init<T>
 * Clients will use this to find and instanciate your module.
 */

/************************************************************************/
/*
 * After a client has created all modules in unspecified order, it
 * will call run() on each of them, also in unspecified order. There
 * probably isn't much difference between doing your initialization
 * in the constructor or run, so I might remove this eventually.
 */

/************************************************************************/

namespace SteamBot
{
    template<typename T> concept ClientModule=std::is_base_of_v<Client::Module, T>;

    class Client::Module
    {
    public:
        class InitBase;
        template <ClientModule T> class Init;

    protected:
        Module();

    public:
        virtual ~Module();
        static void createAll(std::function<void(std::unique_ptr<Client::Module>)>);

    public:
        static SteamBot::Client& getClient() { return SteamBot::Client::getClient(); }

        virtual void run();
    };
}

/************************************************************************/

class SteamBot::Client::Module::InitBase
{
public:
    const InitBase* const next;

protected:
    InitBase();
    virtual ~InitBase();

public:
    virtual std::unique_ptr<Client::Module> create() const =0;
};

/************************************************************************/

template <SteamBot::ClientModule T> class SteamBot::Client::Module::Init : public SteamBot::Client::Module::InitBase
{
public:
    Init() =default;
    virtual ~Init() =default;

    virtual std::unique_ptr<Client::Module> create() const override
    {
        return std::make_unique<T>();
    }
};
