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

#include <thread>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/fiber/operations.hpp>

/************************************************************************/

static thread_local SteamBot::Client* currentClient=nullptr;

/************************************************************************/

static SteamBot::Counter threadCounter;

/************************************************************************/

SteamBot::Client::Client(SteamBot::ClientInfo& clientInfo_)
    : universe{SteamBot::Universe::get(SteamBot::Universe::Type::Public)},
      dataFile(clientInfo_.accountName, SteamBot::DataFile::FileType::Account),
      clientInfo(clientInfo_)
{
	assert(currentClient==nullptr);
	currentClient=this;
}

/************************************************************************/

SteamBot::Client::~Client()
{
	assert(currentClient==this);
	currentClient=nullptr;
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
    Module::createAll([this](std::shared_ptr<Client::Module> module){
        std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
        bool success=modules.try_emplace(std::type_index(typeid(*module)), std::move(module)).second;
        assert(success);	// only one module per type
    });

    std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
    for (auto& item : modules)
    {
        auto module=item.second;
        std::string name=boost::typeindex::type_id_runtime(*module).pretty_name();
        launchFiber(std::move(name), [this, module=std::move(module)](){
            auto cancellation=cancel.registerObject(*(module->waiter));
            module->run(*this);
        });
    }
}

/************************************************************************/

void SteamBot::Client::main()
{
    SteamBot::UI::Thread::outputText(std::string("running client ")+clientInfo.accountName);

    initModules();
    fiberCounter.wait();

    SteamBot::UI::Thread::outputText(std::string("exiting client ")+clientInfo.accountName);
}

/************************************************************************/
/*
 * Start a new client
 */

void SteamBot::Client::launch(SteamBot::ClientInfo& clientInfo)
{
    BOOST_LOG_TRIVIAL(debug) << "Client::launch()";

    std::thread([counter = threadCounter(), &clientInfo]() mutable {
        std::shared_ptr<Client> client;
        client=std::make_shared<Client>(clientInfo);
        clientInfo.setClient(client);

        client->main();

        BOOST_LOG_TRIVIAL(info) << "client quit with mode \"" << SteamBot::enumToStringAlways(client->quitMode) << "\"";
        switch(client->quitMode)
        {
        case QuitMode::None:
        case QuitMode::Quit:
            break;

        case QuitMode::Restart:
            client.reset();
            clientInfo.setClient(client);
            std::this_thread::sleep_for(std::chrono::seconds(15));
            launch(clientInfo);
            break;
        }
    }).detach();
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

SteamBot::Client* SteamBot::Client::getClientPtr()
{
    return currentClient;
}

/************************************************************************/

SteamBot::Client& SteamBot::Client::getClient()
{
    assert(currentClient!=nullptr);
    return *currentClient;
}

/************************************************************************/

void SteamBot::Client::launchFiber(std::string name, std::function<void()> body)
{
	boost::fibers::fiber([this, name=std::move(name), body=std::move(body), counter=fiberCounter()](){
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
        }
        catch(const OperationCancelledException&)
        {
            BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " was cancelled";
        }

        BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " ending; fiber count is currently at " << fiberCounter.getCount();
	}).detach();
}
