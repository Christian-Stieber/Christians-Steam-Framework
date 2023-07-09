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

#include "AcceptTrade.hpp"
#include "Modules/WebSession.hpp"
#include "Web/URLEncode.hpp"

/************************************************************************/
/*
 * Inpired by ASF, DeclineTradeOffer()
 */

/************************************************************************/

namespace
{
    static const boost::urls::url_view baseUrl{"https://steamcommunity.com/tradeoffer"};

    class Params : public SteamBot::Modules::WebSession::PostWithSession
    {
    public:
        Params(SteamBot::TradeOfferID tradeOfferId)
        {
            url=baseUrl;
            url.segments().push_back(std::to_string(toInteger(tradeOfferId)));
            url.segments().push_back("decline");
        }
    };
}

/************************************************************************/
/*
 * Note: I get a response like
 *    {"tradeofferid":"6213482378"}
 * which is the same that was passed in.
 */

bool SteamBot::declineTrade(SteamBot::TradeOfferID tradeOfferId)
{
    auto response=Params(tradeOfferId).execute();
    if (response->query->response.result()==boost::beast::http::status::ok)
    {
        auto string=SteamBot::HTTPClient::parseString(*(response->query));
        BOOST_LOG_TRIVIAL(debug) << "declineTrade response: " << string;
        return true;
    }
    return false;
}
