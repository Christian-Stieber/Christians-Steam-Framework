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
#include <boost/json/value.hpp>

/************************************************************************/

class CCloud_UserFile;

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
                SteamBot::AppID appId;
                std::string name;
                uint32_t totalCount;
                uint64_t totalSize;

            public:
                boost::json::value toJson() const;
            };

        public:
            std::vector<App> apps;

        public:
            Apps();
            ~Apps();

        public:
            void load();

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
                std::vector<std::string> platforms;

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
