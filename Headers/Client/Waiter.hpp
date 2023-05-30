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
    class Waiter
    {
    public:
        class ItemBase
        {
            friend class Waiter;

        private:
            Waiter* waiter;

        public:
            ItemBase(Waiter& waiter_)
                : waiter(&waiter_)
            {
            }

            virtual ~ItemBase() =default;

        public:
            // Note: this should be thread-safe
            void wakeup()
            {
                if (waiter!=nullptr)
                {
                    waiter->wakeup();
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

    public:
        Waiter();
        ~Waiter();

    private:
        // https://stackoverflow.com/questions/47443922/calling-a-member-function-if-it-exists-falling-back-to-a-free-function-and-vice
        template <typename U, typename = void> struct HasStaticCreatedWaiter : std::false_type { };
        template <typename T> struct HasStaticCreatedWaiter<T, std::void_t<decltype(T::value_type::createdWaiter())>> : std::true_type { };

    public:
        template <typename T, typename... ARGS> requires std::is_base_of_v<ItemBase, T> std::shared_ptr<T> createWaiter(ARGS&&... args)
        {
            auto item=std::make_shared<T>(*this, std::forward<ARGS>(args)...);
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
