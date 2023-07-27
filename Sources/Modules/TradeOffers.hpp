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

#include "HTMLParser/Parser.hpp"

#include "Modules/TradeOffers.hpp"

/************************************************************************/

typedef SteamBot::TradeOffers::TradeOffer TradeOffer;
typedef SteamBot::TradeOffers::TradeOffers TradeOffers;

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace TradeOffers
        {
            namespace Internal
            {
                class Parser : public HTMLParser::Parser
                {
                private:
                    ::TradeOffers& result;

                private:
                    TradeOffer::Item* tradeItem=nullptr;
                    decltype(TradeOffer::myItems)* tradeOfferItems=nullptr;

                    std::unique_ptr<TradeOffer> currentTradeOffer;

                public:
                    Parser(std::string_view, ::TradeOffers&);
                    virtual ~Parser() =default;

                private:
                    void setPartner(const HTMLParser::Tree::Element&);
                    bool handleCurrencyAmount(const HTMLParser::Tree::Element&);
                    bool handleItemsBanner(const HTMLParser::Tree::Element&);
                    bool handleTradePartner(const HTMLParser::Tree::Element&);

                    std::function<void(const HTMLParser::Tree::Element&)> handleTradeOfferItems(const HTMLParser::Tree::Element&);
                    std::function<void(const HTMLParser::Tree::Element&)> handleTradeOffer(const HTMLParser::Tree::Element&);
                    std::function<void(const HTMLParser::Tree::Element&)> handleTradeItem(const HTMLParser::Tree::Element&);

                private:
                    virtual std::function<void(const HTMLParser::Tree::Element&)> startElement(const HTMLParser::Tree::Element&) override;
                    virtual void endElement(const HTMLParser::Tree::Element&) override;
                };
            }
        }
    }
}
