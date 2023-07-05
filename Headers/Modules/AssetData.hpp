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
#include "Modules/TradeOffers.hpp"

#include <string>
#include <memory>

#include <boost/json/value.hpp>

/************************************************************************/

class CEconItem_Description;

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace AssetData
        {
            void use();

            class AssetInfo : public SteamBot::AssetKey
            {
            public:
                enum class ItemType { Unknown, TradingCard, Emoticon, Gems };

            public:
                ItemType itemType=ItemType::Unknown;

                std::string name;
                std::string type;
                SteamBot::AppID marketFeeApp=SteamBot::AppID::None;

            public:
                AssetInfo(const CEconItem_Description&);
                virtual ~AssetInfo();

                virtual boost::json::value toJson() const;
            };

            namespace Messageboard
            {
                // The arrays match their counterparts in the offers
                class IncomingTradeOffers
                {
                public:
                    std::shared_ptr<const SteamBot::Modules::TradeOffers::Messageboard::IncomingTradeOffers> offers;

                public:
                    class TradeOfferAssets
                    {
                    public:
                        const SteamBot::Modules::TradeOffers::TradeOffer& offer;

                        std::vector<std::shared_ptr<const AssetInfo>> myItems;
                        std::vector<std::shared_ptr<const AssetInfo>> theirItems;

                    public:
                        TradeOfferAssets(decltype(offer)&);
                        ~TradeOfferAssets();

                        boost::json::value toJson() const;
                    };

                    std::vector<TradeOfferAssets> assets;

                public:
                    IncomingTradeOffers(decltype(offers)&&);
                    ~IncomingTradeOffers();

                    boost::json::value toJson() const;
                };
            }
        }
    }
}
