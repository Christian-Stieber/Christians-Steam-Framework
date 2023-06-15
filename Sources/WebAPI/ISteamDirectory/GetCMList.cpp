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

#include "WebAPI/ISteamDirectory/GetCMList.hpp"
#include "Asio/Asio.hpp"
#include "WebAPI/WebAPI.hpp"
#include "Client/CallbackWaiter.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * Note: the WebAPI response also has a serverlist_websocket (which we
 * don't need right now), a "result" which I don't know what it tells
 * us, and a "message" (same deal).
 *
 * Note: I'll keep the server-addresses as strings instead of decoding
 * them here. At this stage I'm not even sure whether it's always
 * IP-addresses, and it feels like a waste to decode them all when
 * we'll just pick a random one and use that in the end.
 */

/************************************************************************/

typedef SteamBot::WebAPI::ISteamDirectory::GetCMList GetCMList;

/************************************************************************/

static constexpr std::chrono::minutes cacheLifetime{30};

/************************************************************************/

namespace
{
    class CMList
    {
    private:
        std::unordered_map<unsigned int, std::shared_ptr<const GetCMList>> cache;
        std::unordered_multimap<unsigned int, GetCMList::CallbackType> callbacks;

    private:
        CMList() =default;
        ~CMList() =delete;

    private:
        void requestData(unsigned int);
        void receivedData(unsigned int, std::unique_ptr<SteamBot::WebAPI::Query>);

        std::shared_ptr<const GetCMList> getCacheData(unsigned int);

    public:
        void get(unsigned int, GetCMList::CallbackType);

    public:
        static CMList& get();
    };
}

/************************************************************************/

GetCMList::~GetCMList() =default;

/************************************************************************/
/*
 * Response format:
 * {
 *    "response": {
 *       "serverlist": [
 *          "162.254.197.39:27017",
 *          "162.254.197.39:27018",
 *          ...
 *       ],
 *       "serverlist_websockets": [
 *          "ext1-fra1.steamserver.net:27022",
 *          "ext1-fra1.steamserver.net:27019",
 *          ...
 *       ],
 *       "result": 1,
 *       "message": ""
 *     }
 * }
 */

/************************************************************************/

GetCMList::GetCMList(boost::json::value& json)
    : when(decltype(when)::clock::now())
{
    auto serverlistJson=json.at("response").at("serverlist").as_array();
    serverlist.reserve(serverlistJson.size());
    for (auto& value: serverlistJson)
    {
        serverlist.emplace_back(static_cast<std::string_view>(value.as_string()));
    }
    BOOST_LOG_TRIVIAL(info) << "ISteamDirectory/GetCMList returned " << serverlist.size() << " endpoints";
}

/************************************************************************/

CMList& CMList::get()
{
    static CMList& instance=*new CMList;
    return instance;
}

/************************************************************************/

static void postCallback(GetCMList::CallbackType&& callback, std::shared_ptr<const GetCMList> data)
{
    SteamBot::Asio::getIoContext().post([callback=std::move(callback), data=std::move(data)](){
        callback(std::move(data));
    });
}

/************************************************************************/

void CMList::receivedData(unsigned int cellId, std::unique_ptr<SteamBot::WebAPI::Query> query)
{
    BOOST_LOG_TRIVIAL(info) << "received ISteamDirectory/GetCMList";
    if (query->error)
    {
        assert(false);	// Deal with it some other time
    }
    else
    {
        auto data=std::make_shared<const GetCMList>(query->value);

        bool success=cache.try_emplace(cellId, data).second;
        assert(success);

        auto range=callbacks.equal_range(cellId);
        for (auto iterator=range.first; iterator!=range.second; ++iterator)
        {
            assert(iterator->first==cellId);
            postCallback(std::move(iterator->second), data);
        }
        callbacks.erase(range.first, range.second);
        assert(callbacks.count(cellId)==0);
    }
}

/************************************************************************/

void CMList::requestData(unsigned int cellId)
{
    BOOST_LOG_TRIVIAL(info) << "requesting ISteamDirectory/GetCMList";

    auto initiator=[cellId](std::shared_ptr<SteamBot::WaiterBase> waiter) {
        auto query=std::make_unique<SteamBot::WebAPI::Query>("ISteamDirectory", "GetCMList", 1);
        query->set("cellid", static_cast<int>(cellId));
        return SteamBot::WebAPI::perform(std::move(waiter), std::move(query));
    };

    auto completer=[cellId](std::shared_ptr<SteamBot::WebAPI::Query::WaiterType> intermediate) {
        assert(SteamBot::Asio::isThread());
        if (auto result=intermediate->getResult())
        {
            CMList::get().receivedData(cellId, std::move(*result));
            return true;
        }
        return false;
    };

    SteamBot::CallbackWaiter::perform(std::move(initiator), std::move(completer));
}

/************************************************************************/
/*
 * Will expire entry if too old.
 *
 * empty shared_ptr if expired or not available
 */

std::shared_ptr<const GetCMList> CMList::getCacheData(unsigned int cellId)
{
    std::shared_ptr<const GetCMList> result;
    {
        const auto now=std::chrono::system_clock::now();
        auto iterator=cache.find(cellId);
        if (iterator!=cache.end())
        {
            if (iterator->second->when+cacheLifetime>now)
            {
                result=iterator->second;
            }
            else
            {
                cache.erase(iterator);
            }
        }
    }
    return result;
}

/************************************************************************/

void CMList::get(unsigned int cellId, GetCMList::CallbackType callback)
{
    assert(SteamBot::Asio::isThread());

    std::shared_ptr<const GetCMList> data;

    // note: if we have callbacks stored, it means we're currently retrieving the data
    bool noCallbacks;

    {
        auto result=callbacks.equal_range(cellId);
        noCallbacks=(result.first==callbacks.end());
        if (noCallbacks)
        {
            data=getCacheData(cellId);
        }
    }

    if (data)
    {
        postCallback(std::move(callback), std::move(data));
    }
    else
    {
        callbacks.emplace(cellId, std::move(callback));
        if (noCallbacks)
        {
            requestData(cellId);
        }
    }
}

/************************************************************************/

void GetCMList::get(unsigned int cellId, GetCMList::CallbackType callback)
{
    CMList::get().get(cellId, std::move(callback));
}

/************************************************************************/
/*
 * Temporary hack providing the old API
 */

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

std::shared_ptr<const GetCMList> GetCMList::get(unsigned int cellId)
{
    class Stuff
    {
    public:
        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        std::shared_ptr<const GetCMList> result;
    };

    auto stuff=std::make_shared<Stuff>();

    SteamBot::Asio::getIoContext().post([stuff, cellId]() mutable {
        CMList::get().get(cellId, [stuff=std::move(stuff)](std::shared_ptr<const GetCMList> data) {
            {
                std::lock_guard<decltype(stuff->mutex)> lock(stuff->mutex);
                assert(!stuff->result);
                stuff->result=std::move(data);
            }
            stuff->condition.notify_one();
        });
    });

    std::unique_lock<decltype(stuff->mutex)> lock(stuff->mutex);
    stuff->condition.wait(lock, [&stuff](){ return static_cast<bool>(stuff->result); });
    return std::move(stuff->result);
}
