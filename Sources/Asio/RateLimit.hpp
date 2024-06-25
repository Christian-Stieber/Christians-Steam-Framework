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

#include "Asio/HTTPClient.hpp"

#include <queue>

#include <boost/asio/steady_timer.hpp>

/************************************************************************/
/*
 * This provides a rate-limiting queue. You can queue
 * HTPPClient::Query on it, and they will be executed on the Asio
 * thread when their slot comes up.
 *
 * Note: the other RateLimit facility can't be used, because it's
 * blocking. I'll have to see where that's actually used, and whether
 * it's actually needed considering that we only need to rate-limit
 * http requests.
 */

namespace SteamBot
{
    namespace HTTPClient
    {
        class RateLimitQueue
        {
        private:
            typedef std::shared_ptr<Query::WaiterType> WaiterType;

        private:
            const std::chrono::steady_clock::duration schedule;
            boost::asio::steady_timer timer;
            std::queue<WaiterType> queue;
            WaiterType inProgress;

        private:
            void runQuery(WaiterType);
            void startQuery();
            void enqueue(WaiterType);

        public:
            RateLimitQueue(std::chrono::steady_clock::duration);

            RateLimitQueue(const RateLimitQueue&) =delete;
            RateLimitQueue(RateLimitQueue&&) =delete;
            ~RateLimitQueue() =delete;

        public:
            WaiterType perform(std::shared_ptr<SteamBot::WaiterBase>, Query::QueryPtr);
        };
    }
}
