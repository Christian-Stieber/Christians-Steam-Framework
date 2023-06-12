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

#include "Client/Waiter.hpp"

#include <mutex>
#include <condition_variable>
#include <list>
#include <functional>

/************************************************************************/
/*
 * YOU must provide the functions. They will be called on the
 * UI thread.
 *
 * create() -> create the UI to use
 * quit() -> this is called when the user initates a quit.
 */

namespace SteamBot
{
    namespace UI
    {
        class Base;
        std::unique_ptr<Base> create();

        void quit();
    }
}

/************************************************************************/
/*
 * Factories for the various UIs. You'll probably call one of these
 * for in your SteamBot::UI:create() call.
 */

namespace SteamBot
{
    namespace UI
    {
        std::unique_ptr<Base> createConsole();
    }
}

/************************************************************************/
/*
 * UI input operations return a waiter-item.
 *
 * This waiter item is a "one-time" waiter: it will trigger when the
 * result is available, abd be useless afterwards.
 */

namespace SteamBot
{
    namespace UI
    {
        class WaiterBase : public SteamBot::Waiter::ItemBase
        {
        protected:
            mutable std::mutex mutex;
            bool resultAvailable=false;

        public:
            WaiterBase(std::shared_ptr<SteamBot::Waiter>&&);
            virtual ~WaiterBase();

            void completed();		// call this when done

            virtual bool isWoken() const override;

        protected:
            bool isResultValid() const;
        };

        template <typename T> class Waiter : public WaiterBase
        {
        private:
            T result;

        public:
            Waiter(std::shared_ptr<SteamBot::Waiter> waiter)
                : WaiterBase(std::move(waiter))
            {
            }

            virtual ~Waiter() =default;

        public:
            T* getResult()
            {
                return isResultValid() ? &result : nullptr;
            }
        };
    }
}

/************************************************************************/
/*
 * This is what all UIs derive from.
 *
 * Note: while no implementations other than the console-UI exist,
 * I do hope that this API is general enough to cover other UIs.
 *
 * Note: since the UI runs on a separate thread, you actual interface
 * is in Thread.
 */

namespace SteamBot
{
    namespace UI
    {
        class Base
        {
        public:
            typedef std::chrono::system_clock Clock;
            template <typename T> using ResultParam=std::shared_ptr<Waiter<T>>;

        public:
            class ClientInfo
            {
            public:
                std::string accountName;
                Clock::time_point when;

            public:
                ClientInfo();
                ~ClientInfo();
            };

        public:
            enum class PasswordType {
                AccountPassword,
                SteamGuard_EMail,
                SteamGuard_App
            };

        protected:
            Base();
            static void executeOnThread(std::function<void()>&&);

        public:
            virtual ~Base();

            virtual void outputText(ClientInfo&, std::string) =0;
            virtual void requestPassword(ClientInfo&, ResultParam<std::string>, PasswordType, bool(*)(const std::string&)) =0;
        };
    }
}

/************************************************************************/
/*
 * This operates the thread for the UI.
 *
 * This is also where YOU invoke UI functions.
 */

namespace SteamBot
{
    namespace UI
    {
        class Thread
        {
            friend class Base;

        private:
            std::unique_ptr<Base> ui;

            std::mutex mutex;
            std::condition_variable condition;
            std::list<std::function<void()>> queue;

        private:
            Thread();
            static Thread& get();
            decltype(queue)::value_type dequeue();
            void enqueue(decltype(queue)::value_type&&, bool front=false);

        public:
            ~Thread();

        public:
            static bool isThread();

        public:
            static void outputText(std::string);
            static Base::ResultParam<std::string> requestPassword(std::shared_ptr<SteamBot::Waiter>, Base::PasswordType);
        };
    }
}
