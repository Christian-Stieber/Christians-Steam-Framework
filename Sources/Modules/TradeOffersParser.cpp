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

#include "./TradeOffers.hpp"

#include "Helpers/HTML.hpp"
#include "Helpers/ParseNumber.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::TradeOffers::Internal::Parser Parser;

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> Parser::handleTradeOfferItems(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> callback;

    // <div class="tradeoffer_items primaty">
    // <div class="tradeoffer_items secondary">
    if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items"))
    {
        assert(tradeOfferItems==nullptr);
        if (currentTradeOffer)
        {
            const char* myItemsClass=nullptr;
            const char* theirItemsClass=nullptr;
            switch(result.direction)
            {
            case ::TradeOffers::Direction::Incoming:
                myItemsClass="secondary";
                theirItemsClass="primary";
                break;

            case ::TradeOffers::Direction::Outgoing:
                myItemsClass="primary";
                theirItemsClass="secondary";
                break;

            default:
                assert(false);
            }

            if (SteamBot::HTML::checkClass(element, theirItemsClass))
            {
                tradeOfferItems=&currentTradeOffer->theirItems;
                callback=[this](const HTMLParser::Tree::Element&) { tradeOfferItems=nullptr; };
            }
            else if (SteamBot::HTML::checkClass(element, myItemsClass))
            {
                tradeOfferItems=&currentTradeOffer->myItems;
                callback=[this](const HTMLParser::Tree::Element&) { tradeOfferItems=nullptr; };
            }
            else
            {
                currentTradeOffer.reset();
            }
        }
    }

    return callback;
}

/************************************************************************/

bool Parser::handleCurrencyAmount(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer && tradeItem!=nullptr)
    {
        // <div class="item_currency_amount">10</div>

        if (element.name=="div" && SteamBot::HTML::checkClass(element, "item_currency_amount"))
        {
            decltype(tradeItem->amount) amount;
            auto string=SteamBot::HTML::getCleanText(element);
            if (SteamBot::parseNumber(string, amount))
            {
                assert(amount==tradeItem->amount);
            }
            return true;
        }
    }
    return false;
}

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> Parser::handleTradeItem(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> callback;

    if (currentTradeOffer)
    {
        // <div class="trade_item " style="" data-economy-item="classinfo/753/667924416/667076610">

        if (element.name=="div" && SteamBot::HTML::checkClass(element, "trade_item"))
        {
            assert(tradeOfferItems!=nullptr);
            assert(tradeItem==nullptr);

            if (auto dataEconomyItem=element.getAttribute("data-economy-item"))
            {
                tradeItem=tradeOfferItems->emplace_back(std::make_shared<TradeOffer::Item>()).get();
                if (tradeItem->init(*dataEconomyItem))
                {
                    BOOST_LOG_TRIVIAL(debug) << "" << *dataEconomyItem << " -> " << tradeItem->toJson();
                    callback=[this](const HTMLParser::Tree::Element&) {
                        tradeItem=nullptr;
                    };
                }
                else
                {
                    BOOST_LOG_TRIVIAL(info) << "can't parse asset-string \"" << *dataEconomyItem << "\"";
                    tradeItem=nullptr;
                }
            }

            if (tradeItem==nullptr)
            {
                currentTradeOffer.reset();
            }
        }
    }

    return callback;
}

/************************************************************************/

bool Parser::handleItemsBanner(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        // <div class="tradeoffer_items_banner in_escrow">
        if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer_items_banner"))
        {
#if 1
            BOOST_LOG_TRIVIAL(info) << "ignoring trade offer " << toInteger(currentTradeOffer->tradeOfferId) << ": " << SteamBot::HTML::getCleanText(element);
            currentTradeOffer.reset();
#endif
            return true;
        }
    }
    return false;
}

/************************************************************************/

void Parser::setPartner(const HTMLParser::Tree::Element& element)
{
    assert(currentTradeOffer->partner==SteamBot::AccountID::None);
    if (auto dataMiniprofile=element.getAttribute("data-miniprofile"))
    {
        if (!SteamBot::parseNumber(*dataMiniprofile, currentTradeOffer->partner))
        {
            currentTradeOffer.reset();
        }
    }
}

/************************************************************************/

bool Parser::handleTradePartner(const HTMLParser::Tree::Element& element)
{
    if (currentTradeOffer)
    {
        switch(result.direction)
        {
        case ::TradeOffers::Direction::Incoming:
            // <div class="tradeoffer_partner">
            //   <div class="playerAvatar offline" data-miniprofile="<steam32id>">
            if (element.name=="div" && SteamBot::HTML::checkClass(element, "playeravatar"))
            {
                if (auto parent=element.parent)
                {
                    if (parent->name=="div" && SteamBot::HTML::checkClass(*parent, "tradeoffer_partner"))
                    {
                        setPartner(element);
                        return true;
                    }
                }
            }
            break;

        case ::TradeOffers::Direction::Outgoing:
            if (tradeOfferItems==&currentTradeOffer->theirItems)
            {
                // <a class="tradeoffer_avatar playerAvatar tiny offline" href="https://steamcommunity.com/profiles/..." data-miniprofile="<Steam32>">
                if (element.name=="a" && SteamBot::HTML::checkClass(element, "playeravatar"))
                {
                    setPartner(element);
                    return true;
                }
            }
            break;

        default:
            assert(false);
        }
    }
    return false;
}

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> Parser::handleTradeOffer(const HTMLParser::Tree::Element& element)
{
    std::function<void(const HTMLParser::Tree::Element&)> callback;

    // <div class="tradeoffer" id="tradeofferid_6189309615">
    if (element.name=="div" && SteamBot::HTML::checkClass(element, "tradeoffer"))
    {
        assert(!currentTradeOffer);
        if (auto id=element.getAttribute("id"))
        {
            static const char tradeofferid_prefix[]="tradeofferid_";

            std::string_view number(*id);
            if (number.starts_with(tradeofferid_prefix))
            {
                number.remove_prefix(strlen(tradeofferid_prefix));
                SteamBot::TradeOfferID tradeOfferId;
                if (SteamBot::parseNumber(number, tradeOfferId))
                {
                    currentTradeOffer=std::make_unique<TradeOffer>();
                    currentTradeOffer->tradeOfferId=tradeOfferId;

                    callback=[this](const HTMLParser::Tree::Element&) {
                        if (currentTradeOffer)
                        {
                            if (currentTradeOffer->tradeOfferId!=SteamBot::TradeOfferID::None &&
                                currentTradeOffer->partner!=SteamBot::AccountID::None &&
                                (currentTradeOffer->myItems.size()!=0 || currentTradeOffer->theirItems.size()!=0))
                            {
                                auto& item=result.offers[currentTradeOffer->tradeOfferId];
                                assert(!item);
                                item=std::move(currentTradeOffer);
                            }
                            currentTradeOffer.reset();
                        }
                    };
                }
            }
        }
    }

    return callback;
}

/************************************************************************/

Parser::Parser(std::string_view html, ::TradeOffers& result_)
    : HTMLParser::Parser(html), result(result_)
{
}

/************************************************************************/

std::function<void(const HTMLParser::Tree::Element&)> Parser::startElement(const HTMLParser::Tree::Element& element)
{
    typedef std::function<void(const HTMLParser::Tree::Element&)> ResultType;
    typedef ResultType(Parser::*FunctionType)(const HTMLParser::Tree::Element&);

    static const FunctionType functions[]={
        &Parser::handleTradeItem,
        &Parser::handleTradeOfferItems,
        &Parser::handleTradeOffer
    };

    ResultType callback;
    for (auto function : functions)
    {
        callback=(this->*function)(element);
        if (callback)
        {
            break;
        }
    }
    return callback;
}

/************************************************************************/

void Parser::endElement(const HTMLParser::Tree::Element& element)
{
    handleTradePartner(element) || handleItemsBanner(element) || handleCurrencyAmount(element);
}
