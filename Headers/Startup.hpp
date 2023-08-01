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

#include <utility>
#include <memory>
#include <type_traits>

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * This is crapton of template for very little code... not sure
 * whether it makes sense.
 *
 * It's an attempt to make a wrapper for the global-constructor-init
 * scheme.
 */

/************************************************************************/
/*
 * Create a global object Init<Base, Class, Args...>.
 *
 * "Base" is the virtual baseclass for the stuff you're creating,
 * while Class is the actual class.
 *
 * InitBase::initAll(callback, args...) will create an instance of all
 * Classes, with args as constructor parameters. It will call
 * callback(std::unique_ptr<Base>) with each object.
 */

namespace SteamBot
{
    namespace Startup
    {
        template <typename T, typename... ARGS> class InitBase
        {
        private:
            static inline const InitBase* list=nullptr;
            const InitBase* const next;

        protected:
            InitBase()
                : next(list)
            {
                list=this;
            }

        public:
            virtual ~InitBase() =default;

        public:
            virtual std::unique_ptr<T> init(ARGS...) const =0;

        public:
            template <typename FUNC, typename... MYARGS> static void initAll(FUNC callback, MYARGS&&... args)
            {
                for (auto item=list; item!=nullptr; item=item->next)
                {
                    auto module=item->init(std::forward<MYARGS>(args)...);
                    BOOST_LOG_TRIVIAL(debug) << "created " << boost::typeindex::type_id<T>().pretty_name()
                                             << " module " << boost::typeindex::type_id_runtime(*module).pretty_name();
                    callback(std::move(module));
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
        template <typename T, typename U, typename... ARGS> requires std::is_base_of_v<T, U> class Init : public InitBase<T, ARGS...>
        {
        public:
            Init() =default;
            virtual ~Init() =default;

            virtual std::unique_ptr<T> init(ARGS... args) const override
            {
                return std::make_unique<U>(args...);
            }
        };
    }
}
