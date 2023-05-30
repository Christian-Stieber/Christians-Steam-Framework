#include "Client/Messageboard.hpp"
#include "Vector.hpp"

/************************************************************************/

SteamBot::Messageboard::Messageboard() =default;

/************************************************************************/

SteamBot::Messageboard::~Messageboard()
{
    assert(waiters.empty());	// must not destruct while there are still waiters!!
}

/************************************************************************/

SteamBot::Messageboard::WaiterBase::WaiterBase(SteamBot::Waiter& waiter_, SteamBot::Messageboard& messageboard_)
    : ItemBase(waiter_), messageboard(messageboard_)
{
}

/************************************************************************/

SteamBot::Messageboard::WaiterBase::~WaiterBase() =default;

/************************************************************************/
/*
 * Performs "callback" on all waiters for type "key".
 *
 * This will also clean up released waiters.
 *
 * Pass a "nullptr" callback if you only want to perform the cleanup.
 */

void SteamBot::Messageboard::action(const std::type_index& key, std::function<void(std::shared_ptr<WaiterBase>)> callback)
{
    const auto iterator=waiters.find(key);
    if (iterator!=waiters.end())
    {
        SteamBot::erase(iterator->second, [&callback](std::weak_ptr<WaiterBase>& waiter){
            auto locked=waiter.lock();
            if (locked)
            {
                if (callback) callback(locked);
                return false;
            }
            else
            {
                return true;
            }
        });
        if (iterator->second.empty())
        {
            waiters.erase(iterator);
        }
    }
}
