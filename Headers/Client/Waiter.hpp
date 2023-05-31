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
 * A "Waiter" object is a single object that can be notified from
 * different other systems in SteamBot.
 *
 * To use this, create a Waiter instance first, then use
 * createWaiter<T>(...) to attach waiter classes from other subsystems
 * to it (such as Whiteboard::Waiter<U>).
 *
 * Call wait() to block the calling fiber until one of the attached
 * sources wakes it up.
 *
 * Call cancel() to abort ongoing wait() calls. These will throw an
 * OperationCancelledException.
 *
 * If T has a static function "createdWaiter()", it will be called
 * during createWaiter().
 */

/************************************************************************/

namespace SteamBot
{
    class Waiter : public std::enable_shared_from_this<Waiter>
    {
    public:
        class ItemBase
        {
            friend class Waiter;

        private:
            std::weak_ptr<Waiter> waiter;

        public:
            ItemBase(std::shared_ptr<Waiter> waiter_)
                : waiter(std::move(waiter_))
            {
            }

            virtual ~ItemBase() =default;

        public:
            // Note: this should be thread-safe
            void wakeup()
            {
                auto locked=waiter.lock();
                if (locked)
                {
                    locked->wakeup();
                }
            }

        public:
            virtual void install(std::shared_ptr<ItemBase>) { }
            virtual bool isWoken() const =0;
        };

    private:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        bool cancelled=false;

        std::vector<std::weak_ptr<ItemBase>> items;

    private:
        bool isWoken();

    public:
        void wakeup();
        void wait();
        bool wait(std::chrono::milliseconds);
        void cancel();

    protected:
        Waiter();

    public:
        ~Waiter();

    public:
        static std::shared_ptr<Waiter> create();

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
