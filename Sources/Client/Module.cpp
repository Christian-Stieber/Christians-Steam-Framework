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

typedef SteamBot::Client::Module Module;

/************************************************************************/

static const Module::InitBase* modulesInit;

/************************************************************************/

Module::Module()
    : waiter(SteamBot::Waiter::create())
{
}

Module::~Module() =default;

/************************************************************************/

Module::InitBase::InitBase()
    : next(modulesInit)
{
    modulesInit=this;
}

/************************************************************************/

Module::InitBase::~InitBase()
{
    modulesInit=nullptr;	// just in case
}

/************************************************************************/
/*
 * Loops over all Init modules, creates the module, and passes it to
 * your callback.
 */

void Module::createAll(std::function<void(std::shared_ptr<SteamBot::Client::Module>)> callback)
{
    for (const InitBase* init=modulesInit; init!=nullptr; init=init->next)
    {
        auto module=init->create();
        BOOST_LOG_TRIVIAL(debug) << "created client module: " << boost::typeindex::type_id_runtime(*module).pretty_name();
        callback(std::move(module));
    }
}

/************************************************************************/

void Module::run(Client&)
{
}

/************************************************************************/

void Module::waitForLogin()
{
    auto& client=getClient();

    auto myWaiter=SteamBot::Waiter::create();
    auto cancellation=client.cancel.registerObject(*myWaiter);

    typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
    auto loginStatus=client.whiteboard.createWaiter<LoginStatus>(*myWaiter);

    while (loginStatus->get(LoginStatus::LoggedOut)!=LoginStatus::LoggedIn)
    {
        myWaiter->wait();
    }
}
