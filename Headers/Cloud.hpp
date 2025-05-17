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

#pragma once

#include "MiscIDs.hpp"

#include <vector>
#include <cstdint>
#include <string_view>
#include <array>

#include <boost/json/value.hpp>

/************************************************************************/
/*
 * There is some documentation at
 *   https://partner.steamgames.com/doc/webapi/icloudservice
 *
 * We don't use THAT API, but protobufs look extremely similar...
 */

/************************************************************************/
/*
 * Forward declare.
 */

class CCloud_UserFile;	// steammessages_cloud.steamclient.pb.h

/************************************************************************/
/*
 * This is used by some of the cloud APIs. Steam actually just uses a
 * list of strings, but I don't like that.
 */

namespace SteamBot
{
    namespace Cloud
    {
        enum class Platform : uint8_t
        {
            None=0,
            All=1<<0,
            Windows=1<<1,
            MacOS=1<<2,
            Linux=1<<3,
            Android=1<<4,
            iOS=1<<5,
            Switch=1<<6
        };

        // Returns Platform::None if not found
        Platform getPlatform(std::string_view);

        std::vector<std::string_view> getStrings(Platform);
    }
}

/************************************************************************/
/*
 * Obtain the list of apps with cloud files, along with summary
 * statistics (file count, total size).
 */

namespace SteamBot
{
    namespace Cloud
    {
        class Apps
        {
        public:
            class App
            {
            public:
                SteamBot::AppID appId=SteamBot::AppID::None;
                std::string name;
                uint32_t totalCount=0;
                uint64_t totalSize=0;

            public:
                boost::json::value toJson() const;
            };

        public:
            std::vector<App> apps;

        public:
            Apps();
            ~Apps();

        public:
            bool load();

        public:
            boost::json::value toJson() const;
        };
    }
}

/************************************************************************/
/*
 * Obtain a list of files for a specific app.
 */

namespace SteamBot
{
    namespace Cloud
    {
        class Files
        {
        public:
            class File
            {
            public:
                SteamBot::AppID appId;
                uint32_t fileSize;
                std::string fileName;
                std::chrono::system_clock::time_point timestamp;
                Platform platforms;
                std::array<std::byte, 20> fileSha;
                uint64_t creator;
                uint64_t ugcId;
                uint32_t flags;

            public:
                class InvalidFileException {};

                File(const CCloud_UserFile&);
                ~File();

            public:
                boost::json::value toJson() const;
            };

        public:
            std::vector<File> files;

        public:
            Files();
            ~Files();

        public:
            void load(SteamBot::AppID);

        public:
            boost::json::value toJson() const;
        };
    }
}
