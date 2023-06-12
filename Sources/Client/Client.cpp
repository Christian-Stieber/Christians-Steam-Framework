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
#include "Helpers/Destruct.hpp"
#include "UI/UI.hpp"

#include <thread>
#include <condition_variable>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/fiber/operations.hpp>
#include <filesystem>
#include <regex>

#include "boost/examples/fiber/asio/round_robin.hpp"
#include "AsioYield.hpp"

/************************************************************************/

static thread_local SteamBot::Client* currentClient=nullptr;

thread_local boost::fibers::asio::yield_t boost::fibers::asio::yield{};

/************************************************************************/

static SteamBot::Counter threadCounter;

/************************************************************************/

SteamBot::Client::FiberCounter::FiberCounter(SteamBot::Client& client_)
    : client(client_)
{
}

/************************************************************************/

SteamBot::Client::FiberCounter::~FiberCounter() =default;

/************************************************************************/

void SteamBot::Client::FiberCounter::onEmpty()
{
#if 0
    // why does this not end the io_context->run()?
    boost::asio::use_service<boost::fibers::asio::round_robin::service>(*ioContext).shutdown_service();
#else
    client.ioContext->stop();
#endif
}

/************************************************************************/

SteamBot::Client::Client(std::string&& accountName_)
    : accountName(std::move(accountName_)),
      universe{SteamBot::Universe::get(SteamBot::Universe::Type::Public)},
      dataFile{accountName}
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
        bool success=modules.try_emplace(std::type_index(typeid(*module)), std::move(module)).second;
        assert(success);	// only one module per type
    });

    for (auto& module : modules)
    {
        module.second->run();
    }
}

/************************************************************************/

void SteamBot::Client::main()
{
    ioContext=std::make_shared<boost::asio::io_context>();
    boost::fibers::use_scheduling_algorithm<boost::fibers::asio::round_robin>(ioContext);

    SteamBot::UI::Thread::outputText("running client");
    ExecuteOnDestruct atEnd([](){
        SteamBot::UI::Thread::outputText("exiting client");
    });

    initModules();

    while (true)
    {
        try
        {
            // This doesn't exit until all fibers have exited?
            ioContext->run();
            BOOST_LOG_TRIVIAL(debug) << "event loop exit";
            break;
        }
        catch(...)
        {
            if (quitMode==QuitMode::None)
            {
                BOOST_LOG_TRIVIAL(error) << "unhandled client exception: " << boost::current_exception_diagnostic_information();
                quit(false);
                boost::this_fiber::yield();		// ToDo: why do we need this?
            }
        }
    }

    assert(fiberCounter.getCount()==0);
    assert(cancel.empty());
}

/************************************************************************/
/*
 * Start a new client
 */

void SteamBot::Client::launch(std::string&& accountName)
{
    BOOST_LOG_TRIVIAL(debug) << "Client::launch()";

	std::thread([counter=threadCounter(), accountName=std::move(accountName)]() mutable {
        std::unique_ptr<Client> client;
        client.reset(new Client(std::move(accountName)));
        try
        {
            client->main();
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(error) << "unhandled client exception: " << boost::current_exception_diagnostic_information();
        }

        BOOST_LOG_TRIVIAL(info) << "client quit with mode \"" << SteamBot::enumToStringAlways(client->quitMode) << "\"";
        switch(client->quitMode)
        {
        case QuitMode::None:
        case QuitMode::Quit:
            break;

        case QuitMode::Restart:
            accountName=std::move(client->accountName);
            client.reset();
            sleep(15);
            launch(std::move(accountName));
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
		catch(...)
        {
            BOOST_LOG_TRIVIAL(debug) << "uncaught exception in fiber " << fiberName;
            boost::asio::post(*ioContext, [exception=std::current_exception()](){
                std::rethrow_exception(exception);
            });
        }

        BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " ending; fiber count is currently at " << fiberCounter.getCount();
	}).detach();
}

/************************************************************************/
/*
 * Starts all the clients that we know about, by searching
 * the ~/.SteamBot directory for data files.
 */

void SteamBot::Client::launchAll()
{
    bool first=true;
    std::regex regex("Data-(([a-z]|[A-Z]|[0-9]|_)+)\\.json");
    for (auto const& entry: std::filesystem::directory_iterator{"."})
    {
        if (entry.is_regular_file())
        {
            auto filename=entry.path().filename().string();
            std::smatch matchResults;
            if (std::regex_match(filename, matchResults, regex))
            {
                assert(matchResults.size()==3);
                std::string accountName=matchResults[1].str();
                if (!first)
                {
                    first=false;
                    sleep(15);
                }
                launch(std::move(accountName));
            }
        }
    }
}
