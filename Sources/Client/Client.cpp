#include "Client/Client.hpp"
#include "Client/Module.hpp"
#include "Universe.hpp"
#include "Config.hpp"
#include "Exceptions.hpp"

#include <thread>
#include <condition_variable>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/fiber/operations.hpp>

#include "boost/examples/fiber/asio/round_robin.hpp"
#include "AsioYield.hpp"

/************************************************************************/

static thread_local SteamBot::Client* currentClient=nullptr;

thread_local boost::fibers::asio::yield_t boost::fibers::asio::yield{};

/************************************************************************/

class ThreadCount
{
private:
	static inline std::mutex mutex;
	static inline std::condition_variable condition;
	static inline int count=0;

public:
	static void increase()
	{
		std::lock_guard<std::mutex> lock(mutex);
		count+=1;
	}

	static void decrease()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			assert(count>0);
			count-=1;
		}
		condition.notify_one();
	}

public:
	static void wait()
	{
		std::unique_lock<std::mutex> lock(mutex);
		condition.wait(lock, [](){ return count==0; });
	}
};

/************************************************************************/

SteamBot::Client::Client()
    : universe{SteamBot::Universe::get(SteamBot::Universe::Type::Public)},
      dataFile{SteamBot::Config::SteamAccount::get().user}
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

void SteamBot::Client::quit()
{
    if (!quitting)
    {
        BOOST_LOG_TRIVIAL(debug) << "initiating client quit";
        quitting=true;
        cancel.cancel();
    }
}

/************************************************************************/

void SteamBot::Client::initModules()
{
    Module::createAll([this](std::unique_ptr<Client::Module> module){
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
            if (!quitting)
            {
                BOOST_LOG_TRIVIAL(error) << "unhandled client exception: " << boost::current_exception_diagnostic_information();
                quit();
                boost::this_fiber::yield();		// ToDo: why do we need this?
            }
        }
    }

    assert(fiberCount==0);
    assert(cancel.empty());
}

/************************************************************************/
/*
 * Start a new client
 */

void SteamBot::Client::launch()
{
    BOOST_LOG_TRIVIAL(debug) << "Client::launch()";

    ThreadCount::increase();
	std::thread([](){
        std::unique_ptr<Client> client;
        client.reset(new Client);
        try
        {
            client->main();
        }
        catch(...)
        {
            BOOST_LOG_TRIVIAL(error) << "unhandled client exception: " << boost::current_exception_diagnostic_information();
        }
        if (client->whiteboard.has<RestartClient>())
        {
            client.reset();
            launch();
        }
        ThreadCount::decrease();
    }).detach();
}

/************************************************************************/
/*
 * Run all clients, and wait for them to terminate
 */

void SteamBot::Client::waitAll()
{
    ThreadCount::wait();
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
	fiberCount++;
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
		fiberCount--;
        BOOST_LOG_TRIVIAL(debug) << "fiber " << fiberName << " ended; fiberCount is now " << fiberCount;
        if (fiberCount==0)
        {
#if 0
            // why does this not end the io_context->run()?
            boost::asio::use_service<boost::fibers::asio::round_robin::service>(*ioContext).shutdown_service();
#else
            ioContext->stop();
#endif
        }
	}).detach();
}
