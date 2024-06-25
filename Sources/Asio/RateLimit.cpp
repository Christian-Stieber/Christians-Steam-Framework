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

#include "Asio/RateLimit.hpp"
#include "./BasicQuery.hpp"

#include "Asio/Asio.hpp"

/************************************************************************/

typedef SteamBot::HTTPClient::RateLimitQueue RateLimitQueue;

/************************************************************************/

RateLimitQueue::RateLimitQueue(std::chrono::steady_clock::duration schedule_)
    : schedule(schedule_), timer(SteamBot::Asio::getIoContext(), decltype(schedule){})
{
}

/************************************************************************/

void RateLimitQueue::runQuery(RateLimitQueue::WaiterType query)
{
    assert(!inProgress);
    inProgress=std::move(query);

    typedef SteamBot::HTTPClient::Internal::BasicQuery BasicQuery;
    auto myQuery=std::make_shared<BasicQuery>(*(inProgress->setResult()), [this](BasicQuery& basicQuery) {
        assert(basicQuery.query==inProgress->setResult().get());
        inProgress->completed();
        BOOST_LOG_TRIVIAL(debug) << "RateLimitQueue: setting timer to fire in "
                                 << std::chrono::duration_cast<std::chrono::seconds>(schedule).count() << " seconds";
        timer.expires_after(schedule);
        inProgress.reset();
        startQuery();
    });

    SteamBot::HTTPClient::Internal::performWithRedirect(std::move(myQuery));
}

/************************************************************************/
/*
 * Start the next query, if we're not currently working on one and
 * enough time has passed since the previous one.
 */

void RateLimitQueue::startQuery()
{
    BOOST_LOG_TRIVIAL(debug) << "RateLimitQueue::startQuery() with " << queue.size() << " queued requests";
    assert(SteamBot::Asio::isThread());

    if (!inProgress)
    {
        if (!queue.empty())
        {
            timer.cancel();
            if (timer.expiry()<=decltype(timer)::clock_type::now())
            {
                WaiterType query=std::move(queue.front());
                queue.pop();
                runQuery(std::move(query));
            }
            else
            {
                auto seconds=std::chrono::duration_cast<std::chrono::seconds>(timer.expiry()-decltype(timer)::clock_type::now());
                BOOST_LOG_TRIVIAL(debug) << "RateLimitQueue: waiting for timer to expire in " << seconds.count() << " seconds";

                timer.async_wait([this](const boost::system::error_code& error) {
                    BOOST_LOG_TRIVIAL(debug) << "RateLimitQueue: timer fired with error " << error;
                    if (error)
                    {
                        if (error!=boost::asio::error::operation_aborted)
                        {
                            throw boost::system::system_error(error);
                        }
                    }
                    else
                    {
                        startQuery();
                    }
                });
            }
        }
    }
}

/************************************************************************/
/*
 * Enqueue a query, and start it if nothing is currently running.
 *
 * This will push things to the Asio thread, if not already called on
 * it.
 */

void RateLimitQueue::enqueue(RateLimitQueue::WaiterType result)
{
    if (!SteamBot::Asio::isThread())
    {
        SteamBot::Asio::post("RateLimitQueue", [this, result]() mutable {
            enqueue(std::move(result));
        });
    }
    else
    {
        queue.push(std::move(result));
        startQuery();
    }
}

/************************************************************************/
/*
 * This is you entry point. Creates a waiter item and processes the
 * query on the Asio thread.
 */

RateLimitQueue::WaiterType RateLimitQueue::perform(std::shared_ptr<SteamBot::WaiterBase> waiter, HTTPClient::Query::QueryPtr query)
{
    BOOST_LOG_TRIVIAL(info) << "RateLimitQueue::perform \"" << query->url << "\"";

    auto result=waiter->createWaiter<Query::WaiterType>();
    result->setResult()=std::move(query);
    enqueue(result);
    return result;
}
