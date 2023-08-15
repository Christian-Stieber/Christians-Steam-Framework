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

#include "Vector.hpp"

#include <memory>
#include <functional>

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * A "Cancel" object maintains a list of cancelable objects. Clients
 * register such objects and get a registration handle which can
 * be destructed to remove the registration.
 *
 * When cancel() is called on the object, it will cancel all registered
 * objects.
 *
 * Note: this assumes all fibers run on the same thread, so no
 * synchronization is used. We also don't need a wait() call --
 * all cancels() will, eventually, lead to the owning fiber to
 * exit, and the eventloop runs until all fibers are gone.
 */

/************************************************************************/

namespace SteamBot
{
    template<typename T> concept Cancelable=requires(T object) {
        { object.cancel() };
    };
}

/************************************************************************/

namespace SteamBot
{
    class Cancel
    {
    private:
        class ObjectBase;
        std::vector<std::weak_ptr<ObjectBase>> objects;

    private:
        class ObjectBase
        {
        protected:
            ObjectBase() =default;

        public:
            virtual void cancel() =0;
            virtual ~ObjectBase() =default;
        };

    private:
        template <Cancelable T> class Object : public ObjectBase
        {
        private:
            T& object;

        private:
            virtual void cancel() override
            {
                object.cancel();
            }

        public:
            Object(T& object_)
                : object(object_)
            {
            }

            virtual ~Object() =default;
        };

    private:
        // Removes all expired pointers from the objects
        void cleanup()
        {
            SteamBot::erase(objects, [](const decltype(objects)::value_type& item) {
                return item.expired();
            });
        }

    public:
        template <Cancelable T> auto registerObject(T& object)
        {
            auto result=std::make_shared<Object<T>>(object);
            cleanup();
            objects.push_back(result);
            return result;
        }

    public:
        bool empty()
        {
            cleanup();
            return objects.empty();
        }
        
    public:
        void cancel()
        {
            for (auto& item : objects)
            {
                auto object=item.lock();
                if (object)
                {
                    object->cancel();
                }
            }
        }
    };
}
