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

#include "Client/Client.hpp"
#include "Client/Module.hpp"
#include "Universe.hpp"
#include "Exceptions.hpp"
#include "EnumString.hpp"
#include "UI/UI.hpp"
#include "Modules/MultiPacket.hpp"
#include "Modules/Login.hpp"
#include "TypeName.hpp"
#include "Client/Fiber.hpp"
#include "Client/Counter.hpp"

#include <thread>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/fiber/operations.hpp>

/************************************************************************/

static thread_local std::shared_ptr<SteamBot::Client> currentClient;;

/************************************************************************/

static SteamBot::Counter threadCounter;

/************************************************************************/

SteamBot::Client::Client(SteamBot::ClientInfo& clientInfo_)
    : universe{SteamBot::Universe::get(SteamBot::Universe::Type::Public)},
      dataFile(SteamBot::DataFile::get(clientInfo_.accountName, SteamBot::DataFile::FileType::Account)),
      clientInfo(clientInfo_)
{
}

/************************************************************************/

SteamBot::Client::~Client()
{
    BOOST_LOG_TRIVIAL(debug) << "Client::~Client()";
}

/************************************************************************/

void SteamBot::Client::quit(bool restart)
{
    if (quitMode==QuitMode::None)
    {
        quitMode=restart ? QuitMode::Restart : QuitMode::Quit;
        BOOST_LOG_TRIVIAL(debug) << "initiating client quit with mode \"" << SteamBot::enumToStringAlways(quitMode) << "\"";
        cancel.cancel();
    }
}

/************************************************************************/

void SteamBot::Client::initModules()
{
    // pull in some modules
    SteamBot::Modules::MultiPacket::use();
    SteamBot::Modules::Login::use();

    // construct modules
    Module::createAll([this](std::shared_ptr<Client::Module> module){
        std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
        bool success=modules.try_emplace(std::type_index(typeid(*module)), std::move(module)).second;
        assert(success);	// only one module per type
    });

    // Call init on all modules
    // Note: I'm not entirely sure why I have the module mutex, but we
    // can't keep it locked or init would deadlock when trying to
    // access other modules -- which is the main reason I've
    // introduced init, so we can access other modules before run().
    {
        std::vector<std::shared_ptr<Client::Module>> temp;
        {
            std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
            temp.reserve(modules.size());
            for (auto& item : modules)
            {
                temp.push_back(item.second);
            }
        }
        for (const auto& module : temp)
        {
            module->init(*this);
        }
    }

    // run them
    // Note: module mutex is fine here, since we launch fibers.
    {
        std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
        for (auto& item : modules)
        {
            auto module=item.second;
            std::string name=SteamBot::typeName(*module);
            launchFiber(std::move(name), [this, module=std::move(module)](){
                auto cancellation=cancel.registerObject(*(module->waiter));
                module->run(*this);
            });
        }
    }
}

/************************************************************************/

void SteamBot::Client::main()
{
    SteamBot::UI::Thread::outputText(std::string("running client ")+clientInfo.accountName);

    initModules();

    SteamBot::ClientFiber::Properties::counter.wait();

    SteamBot::UI::Thread::outputText(std::string("exiting client ")+clientInfo.accountName);
}

/************************************************************************/
/*
 * Start a new client
 */

void SteamBot::Client::launch(SteamBot::ClientInfo& clientInfo)
{
    if (!clientInfo.setActive(true))
    {
        BOOST_LOG_TRIVIAL(debug) << "Client::launch()";

        std::thread([counter = threadCounter(), &clientInfo]() mutable {
            boost::fibers::use_scheduling_algorithm<ClientFiber::Scheduler>();

            currentClient=std::make_shared<Client>(clientInfo);

            clientInfo.setClient(currentClient);
            currentClient->main();
            clientInfo.setClient(nullptr);

            BOOST_LOG_TRIVIAL(info) << "client quit with mode \"" << SteamBot::enumToStringAlways(currentClient->quitMode) << "\"";
            switch(currentClient->quitMode)
            {
            case QuitMode::None:
            case QuitMode::Quit:
                break;

            case QuitMode::Restart:
                currentClient.reset();
                std::this_thread::sleep_for(std::chrono::seconds(15));
                launch(clientInfo);
                break;
            }
            clientInfo.setActive(false);
        }).detach();
    }
    else
    {
        BOOST_LOG_TRIVIAL(debug) << "client is already active";
    }
}

/************************************************************************/
/*
 * Run all clients, and wait for them to terminate
 */

void SteamBot::Client::waitAll()
{
    threadCounter.wait();
    BOOST_LOG_TRIVIAL(info) << "all clients have quit";
}

/************************************************************************/

std::shared_ptr<SteamBot::Client> SteamBot::Client::getClientShared()
{
    return currentClient;
}

/************************************************************************/

SteamBot::Client* SteamBot::Client::getClientPtr()
{
    return currentClient.get();
}

/************************************************************************/

SteamBot::Client& SteamBot::Client::getClient()
{
    assert(currentClient);
    return *currentClient;
}

/************************************************************************/
/*
 * ToDo: add the name into the properties, and track which fibers
 * are still active instead of just counting?
 */

void SteamBot::Client::launchFiber(std::string name, std::function<void()> body)
{
	boost::fibers::fiber([this, name=std::move(name), body=std::move(body)](){
        std::string fiberName;
        {
            std::stringstream stream;
            stream << "\"" << name << "\" (" << boost::this_fiber::get_id() << ")";
            fiberName=stream.str();
        }

        BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " started";
		try
        {
            body();
            BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " ending";
        }
        catch(const OperationCancelledException&)
        {
            BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " was cancelled";
        }
	}).detach();
}
