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

#include "Modules/UnifiedMessageClient.hpp"
#include "Client/Module.hpp"
#include "Vector.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageClient::Internal::UnifiedMessageBase UnifiedMessageBase;
typedef SteamBot::Modules::UnifiedMessageClient::ServiceMethodResponseMessage ServiceMethodResponseMessage;

/************************************************************************/

namespace
{
    class UnifiedMessageClientModule : public SteamBot::Client::ModuleBase
    {
    private:
        std::vector<UnifiedMessageBase*> messageList;

    public:
        UnifiedMessageClientModule() =default;
        virtual ~UnifiedMessageClientModule() =default;

    public:
        static decltype(messageList)& getMessageList()
        {
            return SteamBot::Client::getClient().getModule<UnifiedMessageClientModule>()->messageList;
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
    auto waiter=SteamBot::Waiter::create();
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(*waiter);

    std::shared_ptr<SteamBot::Messageboard::Waiter<ServiceMethodResponseMessage>> serviceMethodResponseMessage;
    serviceMethodResponseMessage=waiter->createWaiter<decltype(serviceMethodResponseMessage)::element_type>(SteamBot::Client::getClient().messageboard);

    while (true)
    {
        waiter->wait();
        auto message=serviceMethodResponseMessage->fetch();
        if (message->header.proto.jobid_target()==jobId.getValue())
        {
            if (static_cast<SteamBot::ResultCode>(message->header.proto.eresult())!=SteamBot::ResultCode::OK)
            {
                throw Error(std::move(message));
            }
            return message;
        }
    }
}
