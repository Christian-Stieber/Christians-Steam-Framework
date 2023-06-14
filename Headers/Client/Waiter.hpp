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

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/log/trivial.hpp>

#include <vector>

/************************************************************************/
/*
 * I'm not entirely sure that this was a good way to go, but it's
 * there, at least for now.
 *
 * The "waiter" system acts as a event loop: it is a single object
 * that a fiber can "wait" on, and various other components return
 * "waiter items" that can be attached to a waiter.
 *
 * These "waiter items" have their own trigger status, and different
 * APIs for users to retrieve the result.
 *
 * Note: the wakeup() call (to be implemented by subclasses) either
 * gets a pointer to the item that has just called wakeup, or
 * a nullptr if that came from a cancel.
 *
 * To use this, create a Waiter instance first, then use
 * createWaiter<T>(...) to attach waiter items from other subsystems
 * to it (such as Whiteboard::Waiter<U>).
 *
 * If the aforementioned T has a static function "createdWaiter()", it
 * will be called during createWaiter().
 *
 * "Waiter" is the actual waiter class.
 */

/************************************************************************/

#include <memory>
#include <atomic>
#include <vector>

/************************************************************************/

namespace SteamBot
{
    class WaiterBase : public std::enable_shared_from_this<WaiterBase>
    {
    public:
        class ItemBase;

    protected:
        std::atomic<bool> cancelled{false};

    private:
        std::vector<std::weak_ptr<ItemBase>> items;

    public:
        virtual void wakeup(ItemBase*) =0;	// make this threadsafe!
        void cancel();

    protected:
        WaiterBase();
        bool isWoken();

    public:
        virtual ~WaiterBase();

    private:
        // https://stackoverflow.com/questions/47443922/calling-a-member-function-if-it-exists-falling-back-to-a-free-function-and-vice
        template <typename U, typename = void> struct HasStaticCreatedWaiter : std::false_type { };
        template <typename T> struct HasStaticCreatedWaiter<T, std::void_t<decltype(T::value_type::createdWaiter())>> : std::true_type { };

    public:
        template <typename T, typename... ARGS> requires std::is_base_of_v<ItemBase, T> std::shared_ptr<T> createWaiter(ARGS&&... args)
        {
            auto item=std::make_shared<T>(shared_from_this(), std::forward<ARGS>(args)...);
            items.push_back(item);	// we could do this in ItemBase, but lets play it safe in case it's not forwarded from the derived class

            if constexpr (HasStaticCreatedWaiter<T>::value)
            {
                T::value_type::createdWaiter();
            }

            item->install(item);
            return item;
        }
    };
}

/************************************************************************/

class SteamBot::WaiterBase::ItemBase
{
    friend class WaiterBase;

private:
    std::weak_ptr<WaiterBase> waiter;

public:
    ItemBase(std::shared_ptr<WaiterBase>_);
    virtual ~ItemBase();

public:
    void wakeup();		// Note: this is threadsafe

public:
    virtual void install(std::shared_ptr<ItemBase>);
    virtual bool isWoken() const =0;		// Must be threadsafe
};

/************************************************************************/
/*
 * The "Waiter" class is the classic waiter.
 *
 * Call wait() to block the calling fiber until one of the attached
 * items wakes it up.
 *
 * Call cancel() to abort ongoing wait() calls. These will throw an
 * OperationCancelledException.
 */

namespace SteamBot
{
    class Waiter : public WaiterBase
    {
    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;

    private:
        virtual void wakeup(ItemBase*) override;

    protected:
        Waiter();

    public:
        virtual ~Waiter();

        void wait();
        bool wait(std::chrono::milliseconds);

    public:
        static std::shared_ptr<Waiter> create();
    };
}
