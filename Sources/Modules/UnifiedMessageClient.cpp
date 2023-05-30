#include "Client/Module.hpp"
#include "Vector.hpp"
#include "Modules/UnifiedMessageClient.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessageBase UnifiedMessageBase;
typedef SteamBot::Modules::UnifiedMessageClient::Internal::ServiceMethodResponseMessage ServiceMethodResponseMessage;

/************************************************************************/

namespace
{
    class UnifiedMessageClientModule : public SteamBot::Client::Module
    {
    private:
        std::vector<UnifiedMessageBase*> messageList;

    public:
        UnifiedMessageClientModule() =default;
        virtual ~UnifiedMessageClientModule() =default;

    public:
        static decltype(messageList)& getMessageList()
        {
            return SteamBot::Client::getClient().getModule<UnifiedMessageClientModule>().messageList;
        }
    };

    UnifiedMessageClientModule::Init<UnifiedMessageClientModule> init;
}

/************************************************************************/

UnifiedMessageBase::UnifiedMessageBase()
{
    UnifiedMessageClientModule::getMessageList().push_back(this);
}

/************************************************************************/

UnifiedMessageBase::~UnifiedMessageBase()
{
    SteamBot::erase(UnifiedMessageClientModule::getMessageList(), [this](UnifiedMessageBase* message){
        return message==this;
    });
}

/************************************************************************/
/*
 * std::any is a std::shared_ptr<RESPONSE>, or empty if the jobId wasn't found
 */

std::any UnifiedMessageBase::deserialize(const SteamBot::JobID& jobId, SteamBot::Connection::Deserializer& deserializer)
{
    for (auto message : UnifiedMessageClientModule::getMessageList())
    {
        if (message->jobId==jobId)
        {
            if (!message->receivedResponse)
            {
                message->receivedResponse=true;
                return message->deserialize(deserializer);
            }
            break;
        }
    }
    BOOST_LOG_TRIVIAL(error) << "Received a response for jobId " << jobId.getValue() << ", which has no previous request";
    return std::any();
}

/************************************************************************/

std::shared_ptr<const ServiceMethodResponseMessage> UnifiedMessageBase::waitForResponse() const
{
    SteamBot::Waiter waiter;
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(waiter);

    std::shared_ptr<SteamBot::Messageboard::Waiter<ServiceMethodResponseMessage>> serviceMethodResponseMessage;
    serviceMethodResponseMessage=waiter.createWaiter<decltype(serviceMethodResponseMessage)::element_type>(SteamBot::Client::getClient().messageboard);

    while (true)
    {
        waiter.wait();
        auto message=serviceMethodResponseMessage->fetch();
        if (message->header.proto.jobid_target()==jobId.getValue())
        {
            return message;
        }
    }
}
