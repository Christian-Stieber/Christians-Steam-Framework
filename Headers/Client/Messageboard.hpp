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
    };
}

/************************************************************************/

class SteamBot::Messageboard::WaiterBase : public SteamBot::Waiter::ItemBase
{
protected:
    Messageboard& messageboard;

    WaiterBase(SteamBot::Waiter&, Messageboard&);

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
    Waiter(SteamBot::Waiter& waiter_, Messageboard& messageboard_)
        : WaiterBase(waiter_, messageboard_)
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
