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

#include "Cloud.hpp"
#include "SafeCast.hpp"
#include "Modules/UnifiedMessageClient.hpp"

#include "steamdatabase/protobufs/steam/steammessages_cloud.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Cloud::Apps Apps;

/************************************************************************/

Apps::~Apps() =default;
Apps::Apps() =default;

/************************************************************************/

void Apps::load()
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Cloud::EnumerateUserApps)> EnumerateUserAppsInfo;

    std::shared_ptr<EnumerateUserAppsInfo::ResultType> response;
    {
        EnumerateUserAppsInfo::RequestType request;
        response=SteamBot::Modules::UnifiedMessageClient::execute<EnumerateUserAppsInfo::ResultType>("Cloud.EnumerateUserApps#1", std::move(request));
    }

    apps.clear();
    apps.reserve(static_cast<size_t>(response->apps_size()));
    for (int index=0; index<response->apps_size(); index++)
    {
        const auto& appData=response->apps(index);
        if (appData.has_appid() && appData.has_totalcount() && appData.has_totalsize())
        {
            apps.emplace_back(SteamBot::safeCast<SteamBot::AppID>(appData.appid()),
                              SteamBot::safeCast<uint32_t>(appData.totalcount()),
                              SteamBot::safeCast<uint64_t>(appData.totalsize()) );
        }
    }
}

/************************************************************************/

boost::json::value Apps::App::toJson() const
{
    boost::json::object json;
    json["appId"]=SteamBot::toInteger(appId);
    json["totalCount"]=totalCount;
    json["totalSize"]=totalSize;
    return json;
}

/************************************************************************/

boost::json::value Apps::toJson() const
{
    boost::json::array json;
    for (const auto& app: apps)
    {
        json.push_back(app.toJson());
    }
    return json;
}
