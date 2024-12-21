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

/************************************************************************/
/*
 * All this was reverse-engieered from studying ArchiSteamFarma
 */

/************************************************************************/

#include "Modules/WebSession.hpp"
#include "Modules/DiscoveryQueue.hpp"
#include "Helpers/URLs.hpp"
#include "Web/URLEncode.hpp"
#include "UI/UI.hpp"
#include "MiscIDs.hpp"

#include <boost/url/url_view.hpp>

#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

static std::shared_ptr<const Response> makeGenerateRequest()
{
    static const boost::urls::url_view baseUrl("https://store.steampowered.com/explore/generatenewdiscoveryqueue?l=english");

    class Params : public SteamBot::Modules::WebSession::PostWithSession
    {
    public:
        Params()
        {
            url=baseUrl;

            SteamBot::Web::formUrlencode(body, "queuetype", 0);
        }
    };

    return Params().execute();
}

/************************************************************************/

static std::vector<SteamBot::AppID> getQueueApps(std::shared_ptr<const Response> response)
{
    std::vector<SteamBot::AppID> queue;

    auto data=SteamBot::HTTPClient::parseJson(*(response->query));
    BOOST_LOG_TRIVIAL(debug) << data;

    {
        auto& array=data.at("queue").as_array();
        queue.reserve(array.size());
        for (const auto& item : array)
        {
            auto value=item.to_number<std::underlying_type_t<SteamBot::AppID>>();
            queue.push_back(static_cast<SteamBot::AppID>(value));
        }
    }

    {
        auto& appData=data.at("rgAppData").as_object();
        assert(appData.size()==queue.size());

        SteamBot::UI::OutputText output;
        output << "clearing discovery queue: ";
        const char* separator="";
        for (const auto& item : appData)
        {
            output << separator << item.key() << " (" << item.value().at("name").as_string() << ")";
            separator=", ";
        }
    }

    return queue;
}

/************************************************************************/

static std::shared_ptr<const Response> clearItem(SteamBot::AppID appId)
{
    static const boost::urls::url_view baseUrl("https://store.steampowered.com/app?l=english");

    class Params : public SteamBot::Modules::WebSession::PostWithSession
    {
    public:
        Params(SteamBot::AppID appId)
        {
            url=baseUrl;

            const auto value=toUnsignedInteger(appId);
            url.segments().push_back(std::to_string(value));
            SteamBot::Web::formUrlencode(body, "appid_to_clear_from_queue", value);
        }
    };

    return Params(appId).execute();
}

/************************************************************************/

static bool clearQueue(const std::vector<SteamBot::AppID>& queue)
{
    for (const auto appId : queue)
    {
        auto response=clearItem(appId);
        if (response->query->response.result()!=boost::beast::http::status::ok)
        {
            return false;
        }
    }
    return true;
}

/************************************************************************/

static bool performClear()
{
    BOOST_LOG_TRIVIAL(debug) << "DisvoveryQueue.cpp: performClear";
    try
    {
        auto response=makeGenerateRequest();
        if (response->query->response.result()==boost::beast::http::status::ok)
        {
            const auto queue=getQueueApps(std::move(response));
            if (clearQueue(queue))
            {
                SteamBot::UI::OutputText() << "cleared discovery queue";
                return true;
            }
        }
        else
        {
            SteamBot::UI::OutputText() << "error on discovery queue";
        }
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "SaleSticker: exception " << boost::current_exception_diagnostic_information();
    }
    return false;
}

/************************************************************************/

bool SteamBot::DiscoveryQueue::clear()
{
    static thread_local boost::fibers::mutex mutex;
    static thread_local bool lastResult=false;

    std::unique_lock<decltype(mutex)> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        lock.lock();
        return lastResult;
    }
    return lastResult=performClear();
}
