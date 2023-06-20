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

#include "Client/Module.hpp"
#include "Modules/Login.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Client::ModuleBase ModuleBase;

/************************************************************************/

static const ModuleBase::InitBase* modulesInit;

/************************************************************************/

ModuleBase::ModuleBase() =default;
ModuleBase::~ModuleBase() =default;

/************************************************************************/

ModuleBase::InitBase::InitBase()
    : next(modulesInit)
{
    modulesInit=this;
}

/************************************************************************/

ModuleBase::InitBase::~InitBase()
{
    modulesInit=nullptr;	// just in case
}

/************************************************************************/
/*
 * Loops over all Init modules, creates the module, and passes it to
 * your callback.
 */

void ModuleBase::createAll(std::function<void(std::shared_ptr<SteamBot::Client::ModuleBase>)> callback)
{
    for (const InitBase* init=modulesInit; init!=nullptr; init=init->next)
    {
        auto module=init->create();
        BOOST_LOG_TRIVIAL(debug) << "created client module: " << boost::typeindex::type_id_runtime(*module).pretty_name();
        callback(std::move(module));
    }
}

/************************************************************************/
/*
 * This is an internal function to prepare modules, and run()
 * them. Most added for the new Client::Module baseclass.
 */

void ModuleBase::invoke(Client&)
{
    run();
}

/************************************************************************/

void ModuleBase::run()
{
}

/************************************************************************/

typedef SteamBot::Client::Module Module;

/************************************************************************/

Module::Module()
    : waiter(SteamBot::Waiter::create())
{
}

/************************************************************************/

Module::~Module() =default;

/************************************************************************/

void Module::invoke(SteamBot::Client& client)
{
    std::string name=boost::typeindex::type_id_runtime(*this).pretty_name();
    client.launchFiber(std::move(name), [this, &client](){
        auto cancellation=client.cancel.registerObject(*waiter);

        {
            typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
            auto loginStatus=waiter->createWaiter<SteamBot::Whiteboard::Waiter<LoginStatus>>(client.whiteboard);

            while (loginStatus->get(LoginStatus::LoggedOut)!=LoginStatus::LoggedIn)
            {
                waiter->wait();
            }
        }

        run();
    });
}
