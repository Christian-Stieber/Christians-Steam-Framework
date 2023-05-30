#pragma once

#include "Client/Client.hpp"
#include "DestructMonitor.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Connection
        {
            namespace Internal
            {
                class HandlerBase
                {
                protected:
                    HandlerBase();

                public:
                    virtual ~HandlerBase();

                protected:
                    static void add(SteamBot::Connection::Message::Type, std::unique_ptr<HandlerBase>&&);

                private:
                    virtual std::shared_ptr<SteamBot::DestructMonitor> send(SteamBot::Connection::Base::ConstBytes) const =0;

                public:
                    void handle(SteamBot::Connection::Base::ConstBytes) const;
                };
            }
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace Connection
        {
            namespace Internal
            {
                template <typename T> class Handler : public HandlerBase
                {
                public:
                    Handler() =default;
                    virtual ~Handler() =default;

                private:
                    virtual std::shared_ptr<SteamBot::DestructMonitor> send(SteamBot::Connection::Base::ConstBytes bytes) const override
                    {
                        auto message=std::make_shared<T>(bytes);
                        SteamBot::Client::getClient().messageboard.send(message);
                        return message;
                    }

                public:
                    static void create()
                    {
                        add(T::messageType, std::make_unique<Handler<T>>());
                    }
                };
            }
        }
    }
}
