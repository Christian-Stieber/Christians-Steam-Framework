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

#pragma once

#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <memory>
#include <queue>

#include "Waiter.hpp"

/************************************************************************/
/*
 * The messageboard is a hub to send messages to other modules.
 *
 * Messages are identified by their type; you just post a message,
 * without knowing who may or may not receive it.
 *
 * Any number of modules can subscribe to a message-type, if they
 * are interested in it.
 *
 * Note that you can wait for messages, using the same "Waiter"
 * mechanism used for the Whiteboard.
 */

/************************************************************************/

namespace SteamBot
{
    class Messageboard
    {
    public:
        class WaiterBase;
        template <typename T> class Waiter;

    private:
        std::unordered_map<std::type_index, std::vector<std::weak_ptr<WaiterBase>>> waiters;

    private:
        void action(const std::type_index&, std::function<void(std::shared_ptr<WaiterBase>)>);

    public:
        Messageboard();
        ~Messageboard();

        Messageboard(const Messageboard&) =delete;
        Messageboard(Messageboard&&) =delete;

    public:
        template <typename T> unsigned int send(std::shared_ptr<T>);

    public:
        template <typename T> using WaiterType=std::shared_ptr<Waiter<T>>;

        template <typename T> WaiterType<T> createWaiter(SteamBot::Waiter& waiter)
        {
            return waiter.createWaiter<Waiter<T>>(*this);
        }
    };
}

/************************************************************************/

namespace SteamBot
{
    // https://stackoverflow.com/a/61034367

    // Note: when declaring a "concept", we use a "requires
    // expressions" which only needs to be "language correct", or
    // something along those lines.
    template <typename HANDLER, typename MESSAGE> concept MessageHandler =
        requires(HANDLER& handler)
        {
            handler.handle(std::declval<std::shared_ptr<const MESSAGE>>());
        };
}

/************************************************************************/

class SteamBot::Messageboard::WaiterBase : public SteamBot::WaiterBase::ItemBase
{
protected:
    Messageboard& messageboard;

    WaiterBase(std::shared_ptr<SteamBot::WaiterBase>, Messageboard&);

public:
    virtual ~WaiterBase();
};

/************************************************************************/
/*
 * Call fetch() to get the next message.
 * You'll get a nullptr message if there aren't any left.
 */

template <typename T> class SteamBot::Messageboard::Waiter : public SteamBot::Messageboard::WaiterBase
{
public:
    typedef T value_type;

private:
    std::queue<std::shared_ptr<const T>> messages;

public:
    virtual ~Waiter()
    {
        messageboard.action(std::type_index(typeid(T)), nullptr);
    }

public:
    Waiter(std::shared_ptr<SteamBot::WaiterBase> waiter_, Messageboard& messageboard_)
        : WaiterBase(std::move(waiter_), messageboard_)
    {
    }

public:
    virtual void install(std::shared_ptr<ItemBase> item) override
    {
        auto ourItem=std::dynamic_pointer_cast<SteamBot::Messageboard::WaiterBase>(item);
        assert(ourItem);
        messageboard.waiters[std::type_index(typeid(T))].emplace_back(ourItem);
    }

public:
    virtual bool isWoken() const override
    {
        return !messages.empty();
    }

public:
    void received(const std::shared_ptr<T>& message)
    {
        messages.emplace(std::const_pointer_cast<const T, T>(message));
        wakeup();
    }

public:
    std::shared_ptr<const T> fetch()
    {
        std::shared_ptr<const T> message;
        if (!messages.empty())
        {
            message=std::move(messages.front());
            messages.pop();
        }
        return message;
    }

public:
    template <MessageHandler<T> HANDLER> unsigned int handle(HANDLER* handler, bool one=false)
    {
        unsigned int count=0;
        do
        {
            auto message=fetch();
            if (!message) break;
            handler->handle(message);
            count++;
        }
        while (!one);
        return count;
    }
};

/************************************************************************/
/*
 * Sends the message to all currently registered waiters.
 *
 * Returns the number of recipients.
 */

template <typename T> unsigned int SteamBot::Messageboard::send(const std::shared_ptr<T> message)
{
    unsigned int count=0;
    std::type_index key(typeid(T));
    action(key, [&count, &message](std::shared_ptr<WaiterBase> item) {
        auto waiter=std::dynamic_pointer_cast<SteamBot::Messageboard::Waiter<T>>(item);
        assert(waiter);
        waiter->received(message);
        count++;
    });
    return count;
}
