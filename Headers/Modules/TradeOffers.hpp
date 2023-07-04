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

#include <vector>
#include <memory>

#include <boost/json/value.hpp>

#include "Printable.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace TradeOffers
        {
            void use();

            class TradeOffer
            {
            public:
                uint64_t tradeOfferId=0;
                uint32_t partner=0;	/* Steam32 ID */

                std::vector<std::string> myItems;
                std::vector<std::string> theirItems;	// "classinfo/753/667924416/667076610"

            public:
                TradeOffer();
                ~TradeOffer();

                boost::json::value toJson() const;
            };

            class IncomingTradeOffers
            {
            public:
                std::vector<std::unique_ptr<TradeOffer>> offers;

            public:
                IncomingTradeOffers();
                ~IncomingTradeOffers();

                boost::json::value toJson() const;
            };
        }
    }
}
