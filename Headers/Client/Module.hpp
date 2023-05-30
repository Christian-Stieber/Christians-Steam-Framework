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
