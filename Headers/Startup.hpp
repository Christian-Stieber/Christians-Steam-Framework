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

#include "TypeName.hpp"

#include <memory>
#include <type_traits>

#include <boost/intrusive/slist.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * This is a wrapper for the global-constructor-init scheme, whereby
 * classes are registered through a global static object that's put
 * into a list, so you can process that list later.
 *
 * To use this, make a global SteamBot::Startup::Init<BASE,T> object.
 * BASE must be a base class for all your T classes; it creates
 * different lists for each BASE, and object creation returns a BASE
 * pointer.
 *
 * When your program is running, use InitBase<T>::create(callback) to
 * construct an instance of your classes in unspecified order, passing
 * them to the callback.
 *
 * Your class constuctor can have these signaturs:
 *   T()
 *   T(const InitBase<BASE>&)
 *   T(const Init<BASE,T>&)
 */

/************************************************************************/

namespace SteamBot
{
    namespace Startup
    {
        template <typename BASE> class InitBase;

        template <typename FUNC, typename BASE> concept CreateCallback=requires(FUNC func, std::unique_ptr<BASE>&& item) { func(std::move(item)); };
        template <typename PRED, typename BASE> concept CreatePredicate=requires(PRED pred, const InitBase<BASE>& item) { pred(item); };

        template <typename BASE> class InitBase :
            public boost::intrusive::slist_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
        {
        private:
            typedef boost::intrusive::slist<InitBase, boost::intrusive::constant_time_size<false>> ListType;

            static ListType& getList()
            {
                static ListType list;
                return list;
            }

        public:
            InitBase()
            {
                getList().push_front(*this);
            }

            virtual ~InitBase() =default;

        private:
            static constexpr bool alwaysTrue(const InitBase&) { return true; }

        public:
            virtual std::unique_ptr<BASE> createInstance() const =0;

            template <CreateCallback<BASE> FUNC, CreatePredicate<BASE> PRED=decltype(alwaysTrue)> static void create(FUNC callback, PRED predicate=alwaysTrue)
            {
                for (const auto& item: getList())
                {
                    if (predicate(item))
                    {
                        callback(item.createInstance());
                    }
                }
            }
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Startup
    {
        template <typename BASE, typename T> requires std::is_base_of_v<BASE, T> class Init;

        namespace Internal
        {
            template <typename BASE, typename T> concept UsesInitParam =
                requires(const Init<BASE,T>& init)
                {
                    T(init);
                };
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Startup
    {
        template <typename BASE, typename T> requires std::is_base_of_v<BASE, T> class Init : public InitBase<BASE>
        {
        public:
            virtual ~Init() =default;

        public:
            virtual std::unique_ptr<BASE> createInstance() const override
            {
                BOOST_LOG_TRIVIAL(debug) << "creating " << typeName<T>();

                if constexpr (Internal::UsesInitParam<BASE, T>)
                {
                    return std::make_unique<T>(*this);
                }
                else
                {
                    return std::make_unique<T>();
                }
            }
        };
    }
}
