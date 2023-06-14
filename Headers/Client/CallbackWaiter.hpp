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
 * If your final waiter-item is of type RESULT, and you have
 * an intermediate asyc operation of type INTERMEDIATE, your
 * perform() call looks like so:
 *
 * std::shared_ptr<RESULT> perform(std::shared_ptr<WaiterBase> waiter, INITIATOR initiator, COMPLETER completer);
 *
 * Initiator is used like this:
 *    std::shared_ptr<INTERMEDIATE> initiator(std::shared_ptr<WaiterBase>, std::shared_ptr<RESULT>)
 * This lets you create your intermediate async operation.
 *
 * The result is passed for your convenience, in case you want to use
 * it to store something. You're not meant to complete the result here.
 *
 * Note that the waiter argument is a temporary waiter, not the one
 * passed to the perform() call.
 *
 * Completer is used like this:
 *    bool initiator(std::shared_ptr<INTERMEDIATE>, std::shared_ptr<RESULT>);
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
        template <typename RESULT, typename INTERMEDIATE> class CallbackWaiter : public SteamBot::WaiterBase
        {
        public:
            std::shared_ptr<CallbackWaiter> self;
            RESULT result;

            std::function<bool(std::shared_ptr<CallbackWaiter>, ItemBase*)> completer;

            // Note: I'm not currently sure whether there's a race problem
            // when calling initiator() and storing the return value into
            // our intermedaite, so I'm introducting a mutex just in case.
            std::mutex mutex;
            INTERMEDIATE intermediate;

        public:
            CallbackWaiter()
            {
                BOOST_LOG_TRIVIAL(debug) << "created 'CallbackWaiter' class: " << boost::typeindex::type_id<CallbackWaiter>().pretty_name();
            }

            ~CallbackWaiter()
            {
                BOOST_LOG_TRIVIAL(debug) << "destroyed 'CallbackWaiter' class: " << boost::typeindex::type_id<CallbackWaiter>().pretty_name();
            }

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

            // Check the completer signature
            static_assert(std::is_same_v<bool, ReturnType<COMPLETER>>);
            static_assert(std::is_same_v<boost::callable_traits::args_t<COMPLETER>, std::tuple<IntermediateType, ResultType>>);

            auto result=waiter->createWaiter<typename ResultType::element_type>();
            auto myWaiter=std::make_shared<CallbackWaiter<ResultType, IntermediateType>>();
            myWaiter->self=myWaiter;
            myWaiter->result=result;
            myWaiter->completer=[completer=std::move(completer)](std::shared_ptr<CallbackWaiter<ResultType, IntermediateType>> waiter, SteamBot::Waiter::ItemBase* item){
                return completer(waiter->intermediate, waiter->result);
            };
            {
                std::lock_guard<std::mutex> lock(myWaiter->mutex);
                myWaiter->intermediate=initiator(myWaiter, result);
            }
            return result;
        }
    }
}
