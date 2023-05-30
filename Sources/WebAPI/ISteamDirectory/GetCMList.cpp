#include "WebAPI/ISteamDirectory/GetCMList.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::WebAPI::ISteamDirectory::GetCMList GetCMList;

/************************************************************************/

static const std::chrono::minutes cacheLifetime{30};

static boost::fibers::mutex mutex;
static std::unordered_map<unsigned int, std::shared_ptr<const GetCMList>> cache;

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

GetCMList::~GetCMList() =default;

/************************************************************************/

GetCMList::GetCMList(unsigned int cellId)
    : when(std::chrono::system_clock::now())
{
    // Get the data from Steam
    boost::json::value json;
    {
        SteamBot::WebAPI::Request request("ISteamDirectory", "GetCMList", 1);
        request.set("cellid", static_cast<int>(cellId));
        json=request.send();
    }

    // Process the serverlist
    {
        auto serverlistJson=json.at("response").at("serverlist").as_array();
        serverlist.reserve(serverlistJson.size());
        for (auto& value: serverlistJson)
        {
            serverlist.emplace_back(static_cast<std::string_view>(value.as_string()));
        }
    }

    BOOST_LOG_TRIVIAL(info) << "ISteamDirectory/GetCMList returned " << serverlist.size() << " endpoints";
}

/************************************************************************/

std::shared_ptr<const GetCMList> GetCMList::get(unsigned int cellId)
{
    const auto now=std::chrono::system_clock::now();
    std::shared_ptr<const GetCMList> data;

    std::lock_guard<decltype(mutex)> lock(mutex);

    {
        auto iterator=cache.find(cellId);
        if (iterator!=cache.end())
        {
            if (iterator->second->when+cacheLifetime>now)
            {
                data=iterator->second;
            }
            else
            {
                cache.erase(iterator);
            }
        }
    }

    if (!data)
    {
        data=std::make_shared<const GetCMList>(cellId);

        bool success=cache.try_emplace(cellId, data).second;
        assert(success);
    }

    return data;
}
