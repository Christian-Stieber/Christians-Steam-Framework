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

#include "Modules/LicenseList.hpp"

/************************************************************************/
/*
 * Note: a ... Whiteboard::PackageData will be posted when all the
 * updates are completed. You'll need to use the getPackageInfo() to
 * actually get any data.
 *
 * Note that this should not cause problems with threads -- each
 * client should only be interested in the licenses that it gets from
 * Steam, and that follows the strict flow where we receive license
 * data, then update the package data, and then we're done.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace PackageData
        {
            class PackageInfo : public SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier
            {
            public:
                boost::json::object data;	// this is the KeyValue data from "buffer"

                // ToDo: we should probably remove this, as it duplicates data.
                std::vector<SteamBot::AppID> appIds;	// data->appids

            public:
                void setData(boost::json::object);

            public:
                PackageInfo(const boost::json::value&);
                PackageInfo(PackageID);
                virtual ~PackageInfo();

                virtual boost::json::value toJson() const override;
            };

            namespace Whiteboard
            {
                class PackageData
                {
                public:
                    typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses::Ptr LicensePtr;
                    LicensePtr licenses;

                public:
                    PackageData(LicensePtr);

                public:
                    static bool isCurrent();
                };
            }

            std::shared_ptr<const PackageInfo> getPackageInfo(const SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier&);
            std::vector<std::shared_ptr<const PackageInfo>> getPackageInfo(SteamBot::AppID);
        }
    }
}
