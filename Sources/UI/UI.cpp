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

#include "UI/UI.hpp"
#include "Config.hpp"

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

WaiterBase::WaiterBase(std::shared_ptr<SteamBot::Waiter>&& waiter)
    : ItemBase(std::move(waiter))
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
    std::cout << "dequeue" << std::endl;
    while (true)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        condition.wait(lock, [this]() { return !queue.empty(); });
        if (!queue.empty())
        {
            auto item=std::move(queue.front());
            queue.pop();
            return item;
        }
    }
}

/************************************************************************/

Thread::Thread()
{
    std::thread([this]() {
        std::cout << "Thread running" << std::endl;
        isUiThread=true;
        ui=SteamBot::UI::create();
        while (true)
        {
            if (auto item=dequeue())
            {
                item();
            }
        }
    }).detach();
}

Thread::~Thread() =default;

/************************************************************************/

Thread& Thread::get()
{
    static Thread& instance=*new Thread();
    return instance;
}

/************************************************************************/

void Thread::enqueue(std::function<void()>&& operation)
{
    std::cout << "enqueue" << std::endl;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        queue.push(std::move(operation));
    }
    condition.notify_one();
}

/************************************************************************/

Base::ClientInfo::ClientInfo()
    : accountName(SteamBot::Config::SteamAccount::get().user),
      when(Clock::now())
{
}

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

Base::ResultParam<std::string> Thread::requestPassword(std::shared_ptr<SteamBot::Waiter> waiter, Base::PasswordType passwordType)
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
    }
    assert(validator!=nullptr);

    auto item=waiter->createWaiter<Waiter<std::string>>();
    get().enqueue([passwordType, item, validator, clientInfo=Base::ClientInfo()]() mutable {
        get().ui->requestPassword(clientInfo, std::move(item), passwordType, validator);
    });
    return item;
}
