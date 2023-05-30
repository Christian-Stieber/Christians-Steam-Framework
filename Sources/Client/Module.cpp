#include "Client/Module.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

static const SteamBot::Client::Module::InitBase* modulesInit;

/************************************************************************/

SteamBot::Client::Module::Module() =default;
SteamBot::Client::Module::~Module() =default;

/************************************************************************/

SteamBot::Client::Module::InitBase::InitBase()
    : next(modulesInit)
{
    modulesInit=this;
}

/************************************************************************/

SteamBot::Client::Module::InitBase::~InitBase()
{
    modulesInit=nullptr;	// just in case
}

/************************************************************************/
/*
 * Loops over all Init modules, creates the module, and passes it to
 * your callback.
 */

void SteamBot::Client::Module::createAll(std::function<void(std::unique_ptr<SteamBot::Client::Module>)> callback)
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
 * This is called after all modules have been created.
 *
 * Not sure whether we actually need this, since you can also
 * do your init in the constructor, but...
 */

void SteamBot::Client::Module::run()
{
}
