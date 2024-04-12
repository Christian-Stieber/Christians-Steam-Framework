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

#include "JobID.hpp"
#include "Modules/Connection.hpp"

/************************************************************************/
/*
 * This lets you send a query, and wait for the reply. It will wait
 * for the response SPECIFICALLY fro your request, not just any
 * message of the type.
 *
 * ToDo: since UnifiedMessageClient is also dealing with jobIds,
 * I should probably think about making something more generic
 * to catch messages fro specific jobIds.
 */

namespace SteamBot
{
    template <typename RESPONSE, typename REQUEST> std::shared_ptr<const RESPONSE> sendAndWait(std::unique_ptr<REQUEST> request)
    {
        auto& client=SteamBot::Client::getClient();
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=client.cancel.registerObject(*waiter);
        auto responseWaiterItem=client.messageboard.createWaiter<RESPONSE>(*waiter);

        const SteamBot::JobID jobId;

        request->header.proto.set_jobid_source(jobId.getValue());
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(request));

        while (true)
        {
            waiter->wait();
            if (auto message=responseWaiterItem->fetch())
            {
                if (message->header.proto.jobid_target()==jobId.getValue())
                {
                    return message;
                }
            }
        }
    }
}
