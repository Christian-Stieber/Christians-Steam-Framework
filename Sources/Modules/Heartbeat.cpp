#include "Client/Module.hpp"
#include "Modules/Connection.hpp"
#include "Modules/Login.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver_login.hpp"

/************************************************************************/

namespace
{
    class HeartbeatModule : public SteamBot::Client::Module
    {
    public:
        HeartbeatModule() =default;
        virtual ~HeartbeatModule() =default;

        virtual void run() override;
    };

    HeartbeatModule::Init<HeartbeatModule> init;
}

/************************************************************************/

void HeartbeatModule::run()
{
    getClient().launchFiber("HeartbeatModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;

        typedef SteamBot::Modules::Connection::Whiteboard::LastMessageSent LastMessageSent;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LastMessageSent>> lastMessageSent;

        {
            auto& whiteboard=SteamBot::Client::getClient().whiteboard;
            loginStatus=waiter.createWaiter<decltype(loginStatus)::element_type>(whiteboard);
            lastMessageSent=waiter.createWaiter<decltype(lastMessageSent)::element_type>(whiteboard);
        }

        while (true)
        {
            bool timeout=false;
            {
                auto delay=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::HeartbeatInterval>();
                if (delay!=nullptr)
                {
                    timeout=!waiter.wait(*delay);
                }
                else
                {
                    waiter.wait();
                }
            }

            lastMessageSent->has();
            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                if (timeout)
                {
                    auto message=std::make_unique<Steam::CMsgClientHeartBeatMessageType>();
                    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
                }
            }
        }
    });
}
