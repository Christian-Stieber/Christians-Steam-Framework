#pragma once

#include <vector>
#include <memory>
#include <functional>

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
            size_t i=0;
            while (i<objects.size())
            {
                if (objects[i].expired())
                {
                    const size_t last=objects.size()-1;
                    if (i<last)
                    {
                        objects[i]=std::move(objects[last]);
                    }
                    objects.pop_back();
                }
                else
                {
                    i++;
                }
            }
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
