#include "Client/Whiteboard.hpp"
#include "Vector.hpp"

/************************************************************************/

SteamBot::Whiteboard::WaiterBase::WaiterBase(SteamBot::Waiter& waiter_, SteamBot::Whiteboard& whiteboard_)
    : ItemBase(waiter_), whiteboard(whiteboard_)
{
}

/************************************************************************/

SteamBot::Whiteboard::WaiterBase::~WaiterBase() =default;

/************************************************************************/

bool SteamBot::Whiteboard::WaiterBase::isWoken() const
{
    return hasChanged;
}

/************************************************************************/

void SteamBot::Whiteboard::WaiterBase::setChanged()
{
    if (!hasChanged)
    {
        hasChanged=true;
        wakeup();
    }
}

/************************************************************************/

SteamBot::Whiteboard::Whiteboard() =default;

/************************************************************************/

SteamBot::Whiteboard::~Whiteboard()
{
    assert(waiters.empty());	// must not destruct while there are still waiters!!
}

/************************************************************************/
/*
 * Performs "callback" on all waiters for type "key".
 *
 * This will also clean up released waiters.
 *
 * Pass a "nullptr" callback if you only want to perform the cleanup.
 */

void SteamBot::Whiteboard::action(const std::type_index& key, std::function<void(std::shared_ptr<WaiterBase>)> callback)
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

/************************************************************************/

void SteamBot::Whiteboard::didChange(const std::type_index& key)
{
    action(key, [](std::shared_ptr<WaiterBase> item){
        item->setChanged();
    });
}
