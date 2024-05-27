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
#include "Modules/Login.hpp"
#include "Client/Client.hpp"
#include "Helpers/HTML.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/url/parse.hpp>
#include <boost/log/trivial.hpp>

#include "./Header.hpp"

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData BadgeData;

/************************************************************************/

namespace
{
    class BadgePageParser : public HTMLParser::Parser
    {
    private:
        BadgeData& data;
        bool& hasNextPage;

    public:
        BadgePageParser(std::string_view html, BadgeData& data_, bool& hasNextPage_)
            : Parser(html), data(data_), hasNextPage(hasNextPage_)
        {
        }

        virtual ~BadgePageParser() =default;

    private:
        bool handle_data(const HTMLParser::Tree::Element&);
        bool handle_pagebtn(const HTMLParser::Tree::Element&);

    private:
        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            handle_data(element) || handle_pagebtn(element);
        }
    };
}

/************************************************************************/

bool BadgePageParser::handle_data(const HTMLParser::Tree::Element& element)
{
    BadgeData::BadgeInfo info;
    auto appId=info.init(element);
    if (appId!=SteamBot::AppID::None)
    {
        auto result=data.badges.insert({appId, info}).second;
        assert(result);
        return true;
    }
    return false;
}

/************************************************************************/

bool BadgePageParser::handle_pagebtn(const HTMLParser::Tree::Element& element)
{
    // <a class='pagebtn' href="?p=2">&gt;</a>
    // <span class="pagebtn disabled">&lt;</span>

    if (element.name=="a" && SteamBot::HTML::checkClass(element, "pagebtn"))
    {
        if (SteamBot::HTML::getCleanText(element)==">")
        {
            if (auto href=element.getAttribute("href"))
            {
                assert(href->find("?p=")!=std::string::npos || href->find("&p=")!=std::string::npos);
                hasNextPage=true;
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/
/*
 * Returns "true" if there was a next-page button on the page
 */

bool SteamBot::GetPageData::parseBadgePage(std::string_view html, SteamBot::Modules::BadgeData::Whiteboard::BadgeData& data)
{
    bool hasNextPage;
    BadgePageParser(html, data, hasNextPage).parse();
    return hasNextPage;
}
