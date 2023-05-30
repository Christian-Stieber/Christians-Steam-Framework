#pragma once

#include <memory>

/************************************************************************/
/*
 * When used as a base class, lets you get a callback when the object
 * is destructued.
 */

namespace SteamBot
{
    class DestructMonitor
    {
    public:
        class DestructCallback
        {
        public:
            DestructCallback() =default;
            virtual ~DestructCallback() =default;
            virtual void call(const DestructMonitor*) =0;
        };

    public:
        std::weak_ptr<DestructCallback> destructCallback;

    public:
        DestructMonitor();
        virtual ~DestructMonitor();
    };
}
