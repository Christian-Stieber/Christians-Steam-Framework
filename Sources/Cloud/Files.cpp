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
#include "Modules/UnifiedMessageClient.hpp"
#include "SafeCast.hpp"
#include "EnumFlags.hpp"
#include "Helpers/Time.hpp"

#include "steamdatabase/protobufs/steam/steammessages_cloud.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Cloud::Files Files;

/************************************************************************/

Files::Files() =default;
Files::~Files() =default;

Files::File::~File() =default;

/************************************************************************/

Files::File::File(const CCloud_UserFile& userFile)
{
    if (userFile.has_appid() && userFile.has_filename() && userFile.has_timestamp() && userFile.has_file_size())
    {
        appId=SteamBot::safeCast<SteamBot::AppID>(userFile.appid());
        fileSize=userFile.file_size();
        fileName=userFile.filename();
        timestamp=std::chrono::system_clock::from_time_t(SteamBot::safeCast<time_t>(userFile.timestamp()));
        platforms=Platform::None;
        for (int index=0; index<userFile.platforms_to_sync_size(); index++)
        {
            const std::string& name=userFile.platforms_to_sync(index);
            const Platform platform=getPlatform(name);
            if (platform==Platform::None)
            {
                BOOST_LOG_TRIVIAL(error) << "Invalid platform string: \"" << name << "\"";
                throw InvalidFileException{};
            }
            platforms=SteamBot::addEnumFlags(platforms, platform);
        }
    }
    else
    {
        throw InvalidFileException{};
    }
}

/************************************************************************/

boost::json::value Files::File::toJson() const
{
    boost::json::object json;
    json["appId"]=SteamBot::toInteger(appId);
    json["fileName"]=fileName;
    json["fileSize"]=fileSize;
    json["timestamp"]=SteamBot::Time::toString(timestamp);
    if (platforms!=Platform::None)
    {
        auto names=getStrings(platforms);
        assert(!names.empty());

        boost::json::array array;
        for (const auto& name: names)
        {
            array.emplace_back(name);
        }
        json["platforms"]=std::move(array);
    }
    return json;
}

/************************************************************************/

boost::json::value Files::toJson() const
{
    boost::json::array json;
    for (const File& file: files)
    {
        json.emplace_back(file.toJson());
    }
    return json;
}

/************************************************************************/

void Files::load(SteamBot::AppID appId)
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Cloud::EnumerateUserFiles)> EnumerateUserFilesInfo;

    std::shared_ptr<EnumerateUserFilesInfo::ResultType> response;
    {
        EnumerateUserFilesInfo::RequestType request;
        request.set_appid(static_cast<uint32_t>(appId));
        request.set_extended_details(true);
        response=SteamBot::Modules::UnifiedMessageClient::execute<EnumerateUserFilesInfo::ResultType>("Cloud.EnumerateUserFiles#1", std::move(request));
    }

    // ToDo: does it split the response?
    if (response->has_total_files())
    {
        assert(response->total_files()==static_cast<uint32_t>(response->files_size()));
    }

    files.clear();
    files.reserve(static_cast<size_t>(response->files_size()));

    for (int index=0; index<response->files_size(); index++)
    {
        try
        {
            files.emplace_back(response->files(index));
            assert(files.back().appId==appId);
        }
        catch(const File::InvalidFileException&)
        {
        }
    }
}
