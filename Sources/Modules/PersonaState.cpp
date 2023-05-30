#include "Client/Module.hpp"
#include "Steam/PersonaState.hpp"
#include "EnumFlags.hpp"
#include "Modules/Connection.hpp"
#include "Modules/Login.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_friends.hpp"

/************************************************************************/
/*
 * For now, this is hardcoded
 */

static const auto state=SteamBot::PersonaState::Online;
static const auto stateFlags=SteamBot::EPersonaStateFlag::None;

/************************************************************************/

namespace
{
    class PersonaStateModule : public SteamBot::Client::Module
    {
    private:
        void setState();

    public:
        PersonaStateModule() =default;
        virtual ~PersonaStateModule() =default;

        virtual void run() override;
    };

    PersonaStateModule::Init<PersonaStateModule> init;
}

/************************************************************************/

void PersonaStateModule::setState()
{
    auto message=std::make_unique<Steam::CMsgClientChangeStatusMessageType>();
    message->content.set_persona_state(static_cast<uint32_t>(state));
    message->content.set_persona_state_flags(static_cast<uint32_t>(stateFlags));

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void PersonaStateModule::run()
{
    getClient().launchFiber("PersonaStateModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter.createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        while (true)
        {
            waiter.wait();
            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                setState();
            }
        }
    });
}
