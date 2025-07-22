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

#include "Modules/BadgeData.hpp"
#include "Helpers/ParseNumber.hpp"
#include "Helpers/HTML.hpp"

#include "HTMLParser/Tree.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * When you click on the "How do I earn more card drops?" link on
 * a badge page (these exist in both the overview and the detail
 * pages), data from this element is shown.
 *
 * Thus, we can use this for both cases.
 *
 * Note that this does not have "level" information -- but right
 * now, we don't need that anyway.
 */

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData::BadgeInfo BadgeInfo;

/************************************************************************/

boost::json::value BadgeInfo::toJson() const
{
    boost::json::object json;
    json["Received"]=cardsReceived;
    json["Earned"]=cardsEarned;
    return json;
}

/************************************************************************/
/*
 * Process a single "card_drop_info_header" with a string prefix and a
 * number.
 */

static bool prcoessHeader(const HTMLParser::Tree::Element& root, std::string_view prefix, unsigned int& result)
{
    for (const auto& child: root.children)
    {
        auto element=dynamic_cast<const HTMLParser::Tree::Element*>(child.get());
        if (element!=nullptr &&
            element->children.size()==1 &&
            element->name=="div" &&
            SteamBot::HTML::checkClass(*element, "card_drop_info_header"))
        {
            if (auto text=dynamic_cast<const HTMLParser::Tree::Text*>(element->children.front().get()))
            {
                std::string_view string(text->text);
                SteamBot::HTML::trimWhitespace(string);
                if (string.starts_with(prefix))
                {
                    string.remove_prefix(prefix.size());
                    if (SteamBot::parseNumber(string, result))
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/************************************************************************/

static bool removePrefix(std::string_view &string, std::string_view prefix)
{
    if (string.starts_with(prefix))
    {
        string.remove_prefix(prefix.length());
        return true;
    }
    return false;
}

/************************************************************************/

static SteamBot::AppID getAppId(const HTMLParser::Tree::Element& root)
{
    if (root.name=="div" && SteamBot::HTML::checkClass(root, "card_drop_info_dialog"))
    {
        if (auto id=root.getAttribute("id"))
        {
            auto appId=SteamBot::AppID::None;
            unsigned int dummy;
            std::string_view string{*id};
            if (removePrefix(string, "card_drop_info_gamebadge_") &&
                SteamBot::parseNumberPrefix(string, appId) &&
                removePrefix(string, "_") &&
                SteamBot::parseNumberPrefix(string, dummy) &&
                removePrefix(string, "_") &&
                SteamBot::parseNumberPrefix(string, dummy))
            {
                return appId;
            }
        }
    }
    return SteamBot::AppID::None;
}

/************************************************************************/

BadgeInfo::BadgeInfo() =default;
BadgeInfo::~BadgeInfo() =default;

/************************************************************************/

SteamBot::AppID BadgeInfo::init(const HTMLParser::Tree::Element& root)
{
    auto appId=getAppId(root);
    if (appId!=SteamBot::AppID::None)
    {
        if (prcoessHeader(root, "Card drops earned: ", cardsEarned) &&
            prcoessHeader(root, "Card drops received: ", cardsReceived))
        {
            BOOST_LOG_TRIVIAL(info) << "BadgeInfo: " << appId << " has " << cardsEarned
                                    << " cards earned and " << cardsReceived << " cards received";
            // assert(cardsEarned>=cardsReceived);		// doesn't always hold, for some reason
            return appId;
        }
    }
    return SteamBot::AppID::None;
}
