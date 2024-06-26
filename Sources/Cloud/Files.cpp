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
#include "Helpers/HexString.hpp"

#include "steamdatabase/protobufs/steam/steammessages_cloud.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Cloud::Files Files;
typedef SteamBot::Cloud::Platform Platform;

/************************************************************************/

Files::Files() =default;
Files::~Files() =default;

Files::File::~File() =default;

/************************************************************************/

static Platform getPlatforms(const CCloud_UserFile& userFile)
{

    Platform platforms=Platform::None;
    for (int index=0; index<userFile.platforms_to_sync_size(); index++)
    {
        const std::string& name=userFile.platforms_to_sync(index);
        const Platform platform=SteamBot::Cloud::getPlatform(name);
        if (platform==Platform::None)
        {
            BOOST_LOG_TRIVIAL(error) << "Invalid platform string: \"" << name << "\"";
            throw Files::File::InvalidFileException{};
        }
        platforms=SteamBot::addEnumFlags(platforms, platform);
    }
    return platforms;
}

/************************************************************************/

Files::File::File(const CCloud_UserFile& userFile)
{
    if (userFile.has_appid() && userFile.has_filename() && userFile.has_timestamp() && userFile.has_file_size() &&
        userFile.has_steamid_creator() && userFile.has_ugcid() && userFile.has_flags() && userFile.has_file_sha())
    {
        appId=SteamBot::safeCast<SteamBot::AppID>(userFile.appid());
        fileSize=userFile.file_size();
        fileName=userFile.filename();
        timestamp=std::chrono::system_clock::from_time_t(SteamBot::safeCast<time_t>(userFile.timestamp()));
        platforms=getPlatforms(userFile);
        creator=userFile.steamid_creator();
        ugcId=userFile.ugcid();
        flags=userFile.flags();
        if (SteamBot::fromHexString(userFile.file_sha(), fileSha))
        {
            return;
        }
    }
    throw InvalidFileException{};
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
    json["creator"]=creator;
    json["ugcId"]=ugcId;
    json["flags"]=flags;
    json["fileSha"]=SteamBot::makeHexString(fileSha);
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
/*
 * ToDo: is there anything we can do to handle changes to the cloud
 * files when iterating over multiple chunks?
 */

void Files::load(SteamBot::AppID appId)
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Cloud::EnumerateUserFiles)> EnumerateUserFilesInfo;

    files.clear();

    uint32_t fileIndex=0;
    std::shared_ptr<EnumerateUserFilesInfo::ResultType> response;

    do
    {
        response.reset();
        {
            EnumerateUserFilesInfo::RequestType request;
            request.set_appid(static_cast<uint32_t>(appId));
            request.set_extended_details(true);
            request.set_start_index(fileIndex);
            request.set_count(10000);		// ToDo: this doesn't seem to do anything?
            response=SteamBot::Modules::UnifiedMessageClient::execute<EnumerateUserFilesInfo::ResultType>("Cloud.EnumerateUserFiles#1", std::move(request));
        }

        files.reserve(response->total_files());

        const auto fileCount=response->files_size();
        fileIndex+=static_cast<uint32_t>(fileCount);

        for (int index=0; index<fileCount; index++)
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
    while (fileIndex<response->total_files());
}
