#include "Client/Module.hpp"
#include "Modules/Connection.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_uds.hpp"

/************************************************************************/

namespace
{
    class ClientAppListModule : public SteamBot::Client::Module
    {
    private:
        void handle(std::shared_ptr<const Steam::CMsgClientGetClientAppListMessageType>);

    public:
        ClientAppListModule() =default;
        virtual ~ClientAppListModule() =default;

        virtual void run() override;
    };

    ClientAppListModule::Init<ClientAppListModule> init;
}

/************************************************************************/

void ClientAppListModule::handle(std::shared_ptr<const Steam::CMsgClientGetClientAppListMessageType> request)
{
    auto response=std::make_unique<Steam::CMsgClientGetClientAppListResponseMessageType>();
    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(response));
}

/************************************************************************/

void ClientAppListModule::run()
{
    getClient().launchFiber("ClientAppListModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientGetClientAppListMessageType>> clientAppListQueue;
        clientAppListQueue=waiter.createWaiter<decltype(clientAppListQueue)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter.wait();
            handle(clientAppListQueue->fetch());
        }
    });
}
