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

#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>

/************************************************************************/
/*
 * This provides a simple function "CallbackWaiter::perform" taking
 * arguments, designed to easily chain waiter-items.
 *
 * There are two versions of this. One simply gives you a callback
 * at the end; the other one additionally takes a waiter object
 * and returns a result-item that you can wait for.
 *    void perform(INITIATOR initiator, COMPLETER completer);
 *    std::shared_ptr<RESULT> perform(std::shared_ptr<WaiterBase> waiter, INITIATOR initiator, COMPLETER completer);
 *
 * Both have an intermediate waiter item that they use alongside
 * an internal waiter.
 *
 * You supply two callback functions.
 *
 * The "initiator" callback gets the temporary waiter as an argument;
 * create and return the intermediate object that you want to wait for
 * and attach it to that waiter.
 *    std::shared_ptr<INTERMEDIATE> initiator(std::shared_ptr<WaiterBase>);
 *
 * The version with the result item also gets the newly created result
 * item as a second argument; you can use this to store some data in
 * there already, if you need to. You're not meant to complete the
 * result here.
 *    std::shared_ptr<INTERMEDIATE> initiator(std::shared_ptr<WaiterBase>, std::shared_ptr<RESULT>);
 *
 * The "completer" callback is called when the intermediate object
 * triggers. Similar to before, the signatures differ slightly based
 * on whether you have a result item or not:
 *    bool completer(std::shared_ptr<INTERMEDIATE>);
 *    bool completer(std::shared_ptr<INTERMEDIATE>, std::shared_ptr<RESULT>);
 *
 * Note that this is called from the "intermediate" context, so it's not necessarily
 * your thrad/fiber. Examine the intermediate, update the result, and return true
 * if we're done.
 */

/************************************************************************/

namespace SteamBot
{
    namespace CallbackWaiter
    {
        template <typename INTERMEDIATE> class CallbackWaiter : public SteamBot::WaiterBase
        {
        public:
            std::shared_ptr<CallbackWaiter> self;

            std::function<bool(std::shared_ptr<CallbackWaiter>, ItemBase*)> completer;

            // Note: I'm not currently sure whether there's a race problem
            // when calling initiator() and storing the return value into
            // our intermedaite, so I'm introducting a mutex just in case.
            std::mutex mutex;
            INTERMEDIATE intermediate;

        public:
            CallbackWaiter() =default;
            virtual ~CallbackWaiter() =default;

        private:
            virtual void wakeup(ItemBase* item) override
            {
                std::lock_guard<std::mutex>{mutex};	// make sure the intermediate is populated

                if (cancelled)
                {
                    // ToDo: ???
                    self.reset();
                }
                else if (completer(self, item))
                {
                    BOOST_LOG_TRIVIAL(debug) << "completed " << boost::typeindex::type_id_runtime(*this).pretty_name();
                    self.reset();
                }
            }
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace CallbackWaiter
    {
        template <typename FUNC> using ReturnType=boost::callable_traits::return_type_t<FUNC>;
        template <typename FUNC, size_t INDEX> using ArgumentType=typename std::tuple_element<INDEX, boost::callable_traits::args_t<FUNC>>::type;

        template <typename INITIATOR, typename COMPLETER>
        auto perform(std::shared_ptr<WaiterBase> waiter, INITIATOR initiator, COMPLETER completer)
        {
            typedef ArgumentType<INITIATOR, 1> ResultType;
            typedef ReturnType<INITIATOR> IntermediateType;
            typedef CallbackWaiter<IntermediateType> CallbackWaiterType;

            // Check the completer signature
            static_assert(std::is_same_v<bool, ReturnType<COMPLETER>>);
            static_assert(std::is_same_v<boost::callable_traits::args_t<COMPLETER>, std::tuple<IntermediateType, ResultType>>);

            auto result=waiter->createWaiter<typename ResultType::element_type>();
            auto myWaiter=std::make_shared<CallbackWaiterType>();
            myWaiter->self=myWaiter;
            myWaiter->completer=[result, completer=std::move(completer)](std::shared_ptr<CallbackWaiterType> waiter, SteamBot::Waiter::ItemBase* item){
                return completer(waiter->intermediate, result);
            };
            {
                std::lock_guard<std::mutex> lock(myWaiter->mutex);
                myWaiter->intermediate=initiator(myWaiter, result);
            }
            return result;
        }
    }
}

/************************************************************************/
/*
 * This is "waiter" that doesn't supply an item at all -- all your
 * action has to take place inside the "completer".
 *
 * This means the main waiter as been removed from the perform() call,
 * and the intiator/completer don't have the result parameter.
 */

namespace SteamBot
{
    namespace CallbackWaiter
    {
        template <typename FUNC> using ReturnType=boost::callable_traits::return_type_t<FUNC>;
        template <typename FUNC, size_t INDEX> using ArgumentType=typename std::tuple_element<INDEX, boost::callable_traits::args_t<FUNC>>::type;

        template <typename INITIATOR, typename COMPLETER>
        auto perform(INITIATOR initiator, COMPLETER completer)
        {
            typedef ReturnType<INITIATOR> IntermediateType;
            typedef CallbackWaiter<IntermediateType> CallbackWaiterType;

            // Check the completer signature
            static_assert(std::is_same_v<bool, ReturnType<COMPLETER>>);
            static_assert(std::is_same_v<boost::callable_traits::args_t<COMPLETER>, std::tuple<IntermediateType>>);

            auto myWaiter=std::make_shared<CallbackWaiterType>();
            myWaiter->self=myWaiter;
            myWaiter->completer=[completer=std::move(completer)](std::shared_ptr<CallbackWaiterType> waiter, SteamBot::Waiter::ItemBase* item){
                return completer(waiter->intermediate);
            };
            {
                std::lock_guard<std::mutex> lock(myWaiter->mutex);
                myWaiter->intermediate=initiator(myWaiter);
            }
        }
    }
}
