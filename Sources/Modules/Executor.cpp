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
#include "Modules/Executor.hpp"
#include "Exceptions.hpp"

/************************************************************************/
/*
 * for now, let's not be hasty trying to make the Messageboard
 * threadsafe, as that also introduces other races like with
 * Client lifetime.
 *
 * Instead, this is a more complex thing that tries to keep
 * lifetime in mind as well.
 */

/************************************************************************/

typedef std::function<void(SteamBot::Client&)> FunctionType;

/************************************************************************/

namespace
{
    class ExecutorFunction
    {
    public:
        enum class Status { Executing, Run, Killed };

    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        FunctionType function;
        Status status=Status::Executing;

    public:
        ExecutorFunction(decltype(function)&& function_)
            : function(std::move(function_))
        {
        }

    public:
        ~ExecutorFunction() =default;

    public:
        // used by the ExecutorModule
        void setStatus(Status newStatus)
        {
            {
                std::lock_guard<decltype(mutex)> lock(mutex);
                status=newStatus;
            }
            condition.notify_one();
        }

        void execute(SteamBot::Client& client)
        {
            function(client);
            setStatus(Status::Run);
        }

    public:
        // used by the calling thread
        // true if successfuly executed, false if killed
        bool wait()
        {
            std::unique_lock<decltype(mutex)> lock(mutex);
            condition.wait(lock, [this]() { return status!=Status::Executing; });
            return status==Status::Run;
        }
    };
}

/************************************************************************/

namespace
{
    class ExecutorModule : public SteamBot::Client::Module
    {
    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        std::queue<std::shared_ptr<ExecutorFunction>> queue;
        bool cancelled=false;

    private:
        std::shared_ptr<ExecutorFunction> getFunction();
        void wait();

    public:
        void cancel()
        {
            {
                std::lock_guard<decltype(mutex)> lock(mutex);
                cancelled=true;
            }
            condition.notify_one();
        }

    public:
        ExecutorModule() =default;

        virtual ~ExecutorModule()
        {
            assert(queue.empty());
        }

        virtual void run(SteamBot::Client&) override;

    public:
        static bool execute(std::shared_ptr<SteamBot::Client>, FunctionType);
        static bool executeWithFiber(std::shared_ptr<SteamBot::Client>, FunctionType);
    };

    ExecutorModule::Init<ExecutorModule> init;
}

/************************************************************************/

std::shared_ptr<ExecutorFunction> ExecutorModule::getFunction()
{
    std::shared_ptr<ExecutorFunction> result;

    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!queue.empty())
    {
        result=std::move(queue.front());
        queue.pop();
    }

    return result;
}

/************************************************************************/

void ExecutorModule::wait()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this]() {
        return cancelled || !queue.empty();
    });

    if (cancelled)
    {
        throw SteamBot::OperationCancelledException();
    }

    assert(!queue.empty());
}

/************************************************************************/

void ExecutorModule::run(SteamBot::Client& client)
{
    auto cancellation=client.cancel.registerObject(*this);

    try
    {
        while (true)
        {
            wait();
            getFunction()->execute(client);
        }
    }
    catch(...)
    {
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            cancelled=true;
        }

        decltype(queue)::value_type function;
        while ((function=getFunction()))
        {
            function->setStatus(ExecutorFunction::Status::Killed);
        }
        throw;
    }
}

/************************************************************************/

bool ExecutorModule::execute(std::shared_ptr<SteamBot::Client> client, FunctionType function_)
{
    auto module=client->getModule<ExecutorModule>();
    assert(module);

    auto function=std::make_shared<ExecutorFunction>(std::move(function_));

    {
        std::lock_guard<decltype(module->mutex)> lock(module->mutex);
        if (module->cancelled)
        {
            return false;
        }
        module->queue.emplace(function);
    }

    module->condition.notify_one();
    return function->wait();
}

/************************************************************************/

bool ExecutorModule::executeWithFiber(std::shared_ptr<SteamBot::Client> client, FunctionType function_)
{
    return execute(std::move(client), [function_=std::move(function_)](SteamBot::Client& client) mutable {
        client.launchFiber("ExecutorModule::executeWithFiber", [&client, function_=std::move(function_)]() {
            function_(client);
        });
    });
}

/************************************************************************/

bool SteamBot::Modules::Executor::execute(std::shared_ptr<SteamBot::Client> client, FunctionType function)
{
    return ExecutorModule::execute(std::move(client), std::move(function));
}

/************************************************************************/

bool SteamBot::Modules::Executor::executeWithFiber(std::shared_ptr<SteamBot::Client> client, FunctionType function)
{
    return ExecutorModule::executeWithFiber(std::move(client), std::move(function));
}
