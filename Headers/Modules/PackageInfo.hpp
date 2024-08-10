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

#include <memory>
#include <string>
#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace PackageInfo
        {
            class Info
            {
            public:
                typedef std::shared_ptr<const Info> Ptr;

            public:
                std::string packageName;	// empty means we have no name

            public:
                Info(decltype(packageName));
                Info(const boost::json::value&);
                ~Info();

                boost::json::value toJson() const;

            public:
                // This is threadsafe
                static Ptr get(SteamBot::PackageID);
            };
        }
    }
}
