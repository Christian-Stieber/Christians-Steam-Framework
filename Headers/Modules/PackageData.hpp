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
 * This is the package information.
 *
 * waitPackageInfo will look up the package info for you, and also
 * block during an update.
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
                using LicenseIdentifier::LicenseIdentifier;
                PackageInfo(const LicenseIdentifier&);
                virtual ~PackageInfo();
            };

            class PackageInfoFull : public PackageInfo
            {
            public:
                boost::json::object data;	// this is the KeyValue data from "buffer"

            public:
                using PackageInfo::PackageInfo;
                PackageInfoFull(const boost::json::value&);
                virtual ~PackageInfoFull();
                virtual boost::json::value toJson() const override;
            };

            std::shared_ptr<const PackageInfoFull> waitPackageInfo(const SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier&);
        }
    }
}
