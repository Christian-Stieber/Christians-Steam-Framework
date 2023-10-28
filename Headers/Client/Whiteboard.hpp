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
#include <typeindex>
#include <memory>

#include "Waiter.hpp"

/************************************************************************/
/*
 * The whiteboard is a container that can hold arbitrary data that
 * users can also wait for.
 *
 * This is where client modules publish their data for others to use.
 */

/************************************************************************/
/*
 * This is the actual whiteboard. Every client has one.
 *
 * Data-items are identified by type, NOT names. This means you can't
 * sensibly store fundamental items or STL containers into it; please
 * wrap everything into your own unique type.
 *
 * set<T> and clear<T> are used to set a value, or remove it from
 * the whiteboard.
 *
 * For non-scalar types, get<T> returns a reference to the stored item
 * or the supplied default. This reference is valid until a yield.
 *
 * For calar types, get<T> returns a copy of the stored item or the
 * supplied default,
 *
 * has<T> returns a pointer to the stored item, or nullptr.  This
 * pointer is valid until a yield.
 */

/************************************************************************/
/*
 * To wait for items to change, use the Whiteboard::Waiter<T> class.
 *
 * Whiteboard::Waiter<T> works as an item class for SteamBot::Waiter.
 * After creating a Waiter instance, call
 *   auto item=waiter.createWaiter<Whiteboard::Waiter<type>>(whiteboard);
 * to attach a waiter for "type" to the Waiter.
 *
 * This item class has a "changed" flag that will be used by the
 * Waiter instance. When setting a value, the whiteboard will
 * set the changed flag on all attached waiters, causing the
 * Waiter.wait() call to return.
 *
 * The item class also has has/get functions to access the value
 * from the whiteboard; using these instead of querying the whiteboard
 * directly will reset the "changed" flag.
 */

/************************************************************************/

namespace SteamBot
{
    class Whiteboard
    {
    private:
        template <typename T> using CleanType = std::remove_cv_t<std::remove_reference_t<T>>;

    private:
        class ItemBase;
        template <typename T> class Item;

    public:
        class WaiterBase;
        template <typename T> class Waiter;

    private:
        std::unordered_map<std::type_index, std::unique_ptr<ItemBase>> items;
        std::unordered_map<std::type_index, std::vector<std::weak_ptr<WaiterBase>>> waiters;

    private:
        void action(const std::type_index&, std::function<void(std::shared_ptr<WaiterBase>)>);
        void didChange(const std::type_index&);

    public:
        Whiteboard();
        ~Whiteboard();

        Whiteboard(const Whiteboard&) =delete;
        Whiteboard(Whiteboard&&) =delete;

    public:
        template <typename T> void set(T&&);
        template <typename T, typename... ARGS> void set(ARGS&&...);
        template <typename T> void clear();

        // nullptr if not exists
        template <typename T> const T* has() const;

        // assert if not exists
        template <typename T> const T& get() const requires (!std::is_scalar_v<T>);
        template <typename T> T get() const requires (std::is_scalar_v<T>);

		// default value if not exists
        template <typename T> const T& get(const T&) const requires (!std::is_scalar_v<T>);
        template <typename T> T get(T) const requires (std::is_scalar_v<T>);

    public:
        template <typename T> using WaiterType=std::shared_ptr<Waiter<T>>;

        template <typename T> WaiterType<T> createWaiter(SteamBot::Waiter& waiter)
        {
            return waiter.createWaiter<SteamBot::Whiteboard::Waiter<T>>(*this);
        }
    };
}

/************************************************************************/

class SteamBot::Whiteboard::WaiterBase : public SteamBot::WaiterBase::ItemBase
{
private:
    bool hasChanged=false;

protected:
    Whiteboard& whiteboard;

    WaiterBase(std::shared_ptr<SteamBot::WaiterBase>, Whiteboard&);

    void resetChanged()
    {
        hasChanged=false;
    }

public:
    virtual ~WaiterBase();
    virtual bool isWoken() const override;

    void setChanged();
};

/************************************************************************/

template <typename T> class SteamBot::Whiteboard::Waiter : public SteamBot::Whiteboard::WaiterBase
{
public:
    virtual ~Waiter()
    {
        whiteboard.action(std::type_index(typeid(T)), nullptr);
    }

public:
    Waiter(std::shared_ptr<SteamBot::WaiterBase> waiter_, Whiteboard& whiteboard_)
        : WaiterBase(std::move(waiter_), whiteboard_)
    {
    }

    virtual void install(std::shared_ptr<ItemBase> item) override
    {
        auto ourItem=std::dynamic_pointer_cast<SteamBot::Whiteboard::WaiterBase>(item);
        assert(ourItem);
        whiteboard.waiters[std::type_index(typeid(T))].emplace_back(ourItem);

        if (whiteboard.has<T>()!=nullptr)
        {
            setChanged();
        }
    }

public:
    const T* has()
    {
        resetChanged();
        return whiteboard.has<T>();
    }

    const T& get(const T& def) requires (!std::is_scalar_v<T>)
    {
        resetChanged();
        return whiteboard.get<T>(def);
    }

    T get(T def) requires (std::is_scalar_v<T>)
    {
        resetChanged();
        return whiteboard.get<T>(def);
    }

    const T& get() requires (!std::is_scalar_v<T>)
    {
        resetChanged();
        return whiteboard.get<T>();
    }

    T get() requires (std::is_scalar_v<T>)
    {
        resetChanged();
        return whiteboard.get<T>();
    }
};

/************************************************************************/

class SteamBot::Whiteboard::ItemBase
{
protected:
    ItemBase() =default;

public:
    virtual ~ItemBase() =default;
};

/************************************************************************/

template <typename T> class SteamBot::Whiteboard::Item : public SteamBot::Whiteboard::ItemBase
{
public:
    T data;

public:
    template <typename U> Item(U&& data_)
        : data(std::forward<U>(data_))
    {
    }

    virtual ~Item() =default;
};

/************************************************************************/

template <typename T> void SteamBot::Whiteboard::set(T&& data)
{
    std::type_index key(typeid(CleanType<T>));
    auto& item=items[key];
    if (item)
    {
        dynamic_cast<Item<CleanType<T>>*>(item.get())->data=std::forward<T>(data);
    }
    else
    {
        item.reset(new Item<CleanType<T>>(std::forward<T>(data)));
    }
    didChange(key);
}

/************************************************************************/

template <typename T, typename... ARGS> void SteamBot::Whiteboard::set(ARGS&& ...args)
{
    std::type_index key(typeid(CleanType<T>));
    auto& item=items[key];
    if (item)
    {
        dynamic_cast<Item<CleanType<T>>*>(item.get())->data=T(std::forward<ARGS>(args)...);
    }
    else
    {
        item.reset(new Item<CleanType<T>>(std::forward<ARGS>(args)...));
    }
    didChange(key);
}

/************************************************************************/

template <typename T> void SteamBot::Whiteboard::clear()
{
    std::type_index key(typeid(T));
    items.erase(key);
    didChange(key);
}

/************************************************************************/

template <typename T> const T* SteamBot::Whiteboard::has() const
{
    auto iterator=items.find(std::type_index(typeid(T)));
    if (iterator==items.end())
    {
        return nullptr;
    }
    auto foo=iterator->second.get();
    auto item=dynamic_cast<Item<T>*>(foo);
    assert(item!=nullptr);
    return &(item->data);
}

/************************************************************************/

template <typename T> const T& SteamBot::Whiteboard::get(const T& def) const requires (!std::is_scalar_v<T>)
{
    const T* data=has<CleanType<T>>();
    return (data==nullptr) ? def : *data;
}

/************************************************************************/

template <typename T> T SteamBot::Whiteboard::get(T def) const requires (std::is_scalar_v<T>)
{
    const T* data=has<CleanType<T>>();
    return (data==nullptr) ? def : *data;
}

/************************************************************************/

template <typename T> const T& SteamBot::Whiteboard::get() const requires (!std::is_scalar_v<T>)
{
    const T* data=has<CleanType<T>>();
    assert(data!=nullptr);
    return *data;
}

/************************************************************************/

template <typename T> T SteamBot::Whiteboard::get() const requires (std::is_scalar_v<T>)
{
    const T* data=has<CleanType<T>>();
    assert(data!=nullptr);
    return *data;
}
