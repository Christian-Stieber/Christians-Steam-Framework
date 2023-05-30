#include "DestructMonitor.hpp"

/************************************************************************/

SteamBot::DestructMonitor::DestructMonitor() =default;

/************************************************************************/

SteamBot::DestructMonitor::~DestructMonitor()
{
    auto locked=destructCallback.lock();
    if (locked)
    {
        locked->call(this);
    }
}
