#pragma once

#include "Client/Waiter.hpp"

/************************************************************************/
/*
 * For now, I'll ignore the idea of having multiple UIs, or UIs that
 * can be selected at runtime. This will be done later.
 *
 * For now, I'll only implement the console/CLI UI, to get some
 * experience what I actually need.
 */

/************************************************************************/

namespace SteamBot
{
    namespace UI
    {
        class UIWaiterBase : public SteamBot::Waiter::ItemBase
        {
        protected:
            std::weak_ptr<UIWaiterBase> self;

        public:
            UIWaiterBase(SteamBot::Waiter&);
            virtual ~UIWaiterBase();

            virtual void install(std::shared_ptr<ItemBase>) override;
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace UI
    {
        class GetSteamguardCode : public UIWaiterBase
        {
        private:
            class Params
            {
            public:
                std::mutex mutex;
                std::string user;
                std::string code;
                volatile bool abort=false;
            };

            std::shared_ptr<Params> params;

        private:
            void execute();

        public:
            GetSteamguardCode(SteamBot::Waiter&);
            virtual ~GetSteamguardCode();

        public:
            virtual bool isWoken() const;

        public:
            std::string fetch();

        public:
            static std::shared_ptr<GetSteamguardCode> create(SteamBot::Waiter&);
        };
    }
}
