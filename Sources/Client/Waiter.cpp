#include "Client/Waiter.hpp"
#include "Vector.hpp"
#include "Exceptions.hpp"

/************************************************************************/

SteamBot::Waiter::Waiter() =default;

/************************************************************************/

SteamBot::Waiter::~Waiter()
{
    SteamBot::erase(items, [](std::weak_ptr<ItemBase>& item){
        auto locked=item.lock();
        if (locked)
        {
            locked->waiter=nullptr;
            return false;
        }
        else
        {
            return true;
        }
    });
}

/************************************************************************/
/*
 * Note: this should be thread-safe
 */

void SteamBot::Waiter::wakeup()
{
    condition.notify_all();
}

/************************************************************************/

void SteamBot::Waiter::cancel()
{
    if (!cancelled)
    {
        cancelled=true;
        wakeup();
    }
}

/************************************************************************/

bool SteamBot::Waiter::isWoken()
{
    bool woken=false;
    SteamBot::erase(items, [&woken](const std::weak_ptr<ItemBase>& item){
        auto locked=item.lock();
        if (locked)
        {
            woken|=locked->isWoken();
            return false;
        }
        else
        {
            return true;
        }
    });
    return woken;
}

/************************************************************************/

void SteamBot::Waiter::wait()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this](){ return cancelled || isWoken(); });
    if (cancelled)
    {
        throw OperationCancelledException();
    }
}

/************************************************************************/
/*
 * Returne false if the timeout was reached
 */

bool SteamBot::Waiter::wait(std::chrono::milliseconds timeout)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    bool result=condition.wait_for(lock, timeout, [this](){ return cancelled || isWoken(); });
    if (cancelled)
    {
        throw OperationCancelledException();
    }
    return result;
}
