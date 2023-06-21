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
#include "Steam/KeyValue.hpp"

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
            class PackageInfo
            {
            public:
                std::shared_ptr<SteamBot::Modules::LicenseList::Whiteboard::Licenses::LicenseInfo> license;

            public:
                class Data
                {
                public:
                    uint32_t changeNumber=0;
                    Steam::KeyValue::Node data;
                };

                std::unique_ptr<const Data> data;
            };

            std::shared_ptr<const PackageInfo> waitPackageInfo(PackageID);
        }
    }
}
