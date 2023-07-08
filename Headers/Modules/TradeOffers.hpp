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

#include "AssetKey.hpp"

#include <vector>
#include <memory>

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
                class Item : public SteamBot::AssetKey
                {
                public:
                    uint32_t amount=0;

                public:
                    Item();
                    virtual ~Item();

                    bool init(std::string_view);

                    virtual boost::json::value toJson() const override;
                };

            public:
                SteamBot::TradeOfferID tradeOfferId=SteamBot::TradeOfferID::None;
                SteamBot::AccountID partner=SteamBot::AccountID::None;

                std::vector<std::shared_ptr<Item>> myItems;
                std::vector<std::shared_ptr<Item>> theirItems;

            public:
                TradeOffer();
                ~TradeOffer();

                boost::json::value toJson() const;
            };

            namespace Messageboard
            {
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
}
