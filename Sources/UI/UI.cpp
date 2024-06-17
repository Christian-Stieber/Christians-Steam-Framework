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

#include "Client/ClientInfo.hpp"
#include "UI/UI.hpp"

/************************************************************************/

typedef SteamBot::UI::Thread Thread;
typedef SteamBot::UI::Base Base;
typedef SteamBot::UI::WaiterBase WaiterBase;
template <typename T> using Waiter=SteamBot::UI::Waiter<T>;

/************************************************************************/

static thread_local bool isUiThread=false;

/************************************************************************/

Base::Base() =default;
Base::~Base() =default;

/************************************************************************/

void Base::quit()
{
}

/************************************************************************/
/*
 * This pushes "function" to the front of the queue.
 */

void Base::executeOnThread(std::function<void()>&& function)
{
    Thread::get().enqueue(std::move(function), true);
}

/************************************************************************/

WaiterBase::WaiterBase(std::shared_ptr<SteamBot::WaiterBase>&& waiter_)
    : ItemBase(std::move(waiter_))
{
}

WaiterBase::~WaiterBase() =default;

/************************************************************************/

bool WaiterBase::isWoken() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return resultAvailable;
}

/************************************************************************/

void WaiterBase::completed()
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        assert(!resultAvailable);
        resultAvailable=true;
    }
    wakeup();
}

/************************************************************************/
/*
 * This checks whether you can access the result.
 *
 * If we're the UI-thread, we can only access it until we completed().
 * If we're not the UI-thread, we can access it after completed() has
 * been called.
 */

bool WaiterBase::isResultValid() const
{
    return isUiThread!=resultAvailable;
}

/************************************************************************/

decltype(Thread::queue)::value_type Thread::dequeue()
{
    while (true)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        condition.wait(lock, [this]() { return !queue.empty(); });
        if (!queue.empty())
        {
            auto item=std::move(queue.front());
            queue.pop_front();
            return item;
        }
    }
}

/************************************************************************/

Thread::Thread()
{
    thread=std::thread([this]() {
        BOOST_LOG_TRIVIAL(debug) << "UI thread running";
        isUiThread=true;
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            ui=SteamBot::UI::create();
        }
        while (true)
        {
            if (auto item=dequeue())
            {
                item();
            }
            else
            {
                break;
            }
        }
        BOOST_LOG_TRIVIAL(debug) << "UI thread terminating";

        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            ui.reset();
            assert(!didQuit);
            didQuit=true;
        }
        condition.notify_all();
    });
}

/************************************************************************/

Thread::~Thread()
{
    thread.join();
}

/************************************************************************/

Thread& Thread::get()
{
    static Thread& instance=*new Thread();
    return instance;
}

/************************************************************************/

void Thread::enqueue(std::function<void()>&& operation, bool front)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (front)
        {
            queue.push_front(std::move(operation));
        }
        else
        {
            queue.push_back(std::move(operation));
        }
    }
    condition.notify_all();
}

/************************************************************************/

Base::ClientInfo::ClientInfo()
    :  when(Clock::now())
{
    if (auto client=SteamBot::Client::getClientPtr())
    {
        accountName=client->getClientInfo().displayName();
    }
}

/************************************************************************/

Base::ClientInfo::~ClientInfo() =default;

/************************************************************************/
/*
 * Adds a '\n' at the end, if not already there.
 */

void Thread::outputText(std::string text)
{
    if (!text.ends_with("\n"))
    {
        text.push_back('\n');
    }

    get().enqueue([text=std::move(text), clientInfo=Base::ClientInfo()]() mutable {
        get().ui->outputText(clientInfo, std::move(text));
    });
}

/************************************************************************/

static bool SteamGuardValidator(const std::string& code)
{
    if (code.size()!=5)
    {
        return false;
    }
    for (char c: code)
    {
        if (!((c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z')))
        {
            return false;
        }
    }
    return true;
}

/************************************************************************/

static bool PasswordValidator(const std::string& password)
{
    return !password.empty();
}

/************************************************************************/

Base::ResultParam<std::string> Thread::requestPassword(std::shared_ptr<SteamBot::WaiterBase> waiter, Base::PasswordType passwordType)
{
    bool(*validator)(const std::string&)=nullptr;
    switch(passwordType)
    {
    case Base::PasswordType::AccountPassword:
        validator=&PasswordValidator;
        break;

    case Base::PasswordType::SteamGuard_EMail:
    case Base::PasswordType::SteamGuard_App:
        validator=&SteamGuardValidator;
        break;

    default:
        assert(false);
    }
    assert(validator!=nullptr);

    auto item=waiter->createWaiter<Waiter<std::string>>();
    get().enqueue([passwordType, item, validator, clientInfo=Base::ClientInfo()]() mutable {
        get().ui->requestPassword(clientInfo, std::move(item), passwordType, validator);
    });
    return item;
}

/************************************************************************/

bool Thread::isThread()
{
    return isUiThread;
}

/************************************************************************/

void Thread::quit()
{
    Thread& thread=get();
    {
        std::lock_guard<decltype(thread.mutex)> lock(thread.mutex);
        if (thread.ui)
        {
            thread.ui->quit();
        }
    }
    thread.enqueue(nullptr, true);
}

/************************************************************************/

void Thread::wait_()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this](){ return didQuit; });
}

/************************************************************************/
/*
 * Wait for UI to quit
 */

void Thread::wait()
{
    get().wait_();
}

/************************************************************************/

SteamBot::UI::OutputText::OutputText() =default;

SteamBot::UI::OutputText::~OutputText()
{
    Thread::outputText(std::move(*this).str());
}
