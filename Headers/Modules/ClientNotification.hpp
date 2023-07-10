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

#include <chrono>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace ClientNotification
        {
            void use();

            namespace Messageboard
            {
                class ClientNotification
                {
                public:
                    // Note: it seems SteamKit and ASF don't handle these, so I have no values from them
                    enum class Type {
                        Unknown=-1,
                        InventoryItem=4,		// {"app_id":"753","context_id":"6","asset_id":"26015473118"}
                        SaleAnnouncement=6,
                        TradeOffer=9			// {"sender":"64837198","tradeoffer_id":"6189309615","comment":""}
                    };

                public:
                    std::chrono::system_clock::time_point timestamp;
                    std::chrono::system_clock::time_point expiry;

                    NotificationID notificationId=NotificationID::None;

                    Type type=Type::Unknown;
                    bool read=false;
                    boost::json::value body;

                    // no idea what these might try to tell us
                    uint32_t targets=0;
                    bool hidden=false;

                public:
                    ClientNotification();
                    ~ClientNotification();

                    boost::json::value toJson() const;
                };
            }
        }
    }
}
