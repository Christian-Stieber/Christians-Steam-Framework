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

#include "Client/Module.hpp"
#include "Modules/WebSession.hpp"
#include "Modules/SaleSticker.hpp"
#include "Random.hpp"

#include <regex>

#include <boost/url/url_view.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    class SaleStickerModule : public SteamBot::Client::Module
    {
    private:
        static std::string getIoken();

    public:
        SaleStickerModule() =default;
        virtual ~SaleStickerModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    SaleStickerModule::Init<SaleStickerModule> init;
}

/************************************************************************/
/*
 * Let's pretend to be a user -- I'll select one randomly :-)
 * I should probably try to get this list from the store, though...
 */

static const std::array<boost::urls::url_view, 20> categoryUrls={{
        boost::urls::url_view("https://store.steampowered.com/category/casual"),
        boost::urls::url_view("https://store.steampowered.com/category/fighting_martial_arts"),
        boost::urls::url_view("https://store.steampowered.com/greatondeck"),
        boost::urls::url_view("https://store.steampowered.com/category/simulation"),
        boost::urls::url_view("https://store.steampowered.com/category/visual_novel"),
        boost::urls::url_view("https://store.steampowered.com/genre/Free%20to%20Play"),
        boost::urls::url_view("https://store.steampowered.com/category/action"),
        boost::urls::url_view("https://store.steampowered.com/vr"),
        boost::urls::url_view("https://store.steampowered.com/category/survival"),
        boost::urls::url_view("https://store.steampowered.com/category/exploration_open_world"),
        boost::urls::url_view("https://store.steampowered.com/category/strategy"),
        boost::urls::url_view("https://store.steampowered.com/category/rpg"),
        boost::urls::url_view("https://store.steampowered.com/category/anime"),
        boost::urls::url_view("https://store.steampowered.com/category/rogue_like_rogue_lite"),
        boost::urls::url_view("https://store.steampowered.com/category/adventure"),
        boost::urls::url_view("https://store.steampowered.com/category/sports"),
        boost::urls::url_view("https://store.steampowered.com/category/story_rich"),
        boost::urls::url_view("https://store.steampowered.com/category/science_fiction"),
        boost::urls::url_view("https://store.steampowered.com/category/multiplayer_coop"),
        boost::urls::url_view("https://store.steampowered.com/category/horror")
    }};

/************************************************************************/
/*
 * Note: my HTML parser can't (currently?) parse these pages, but
 * I don't really need to.
 */

std::string SaleStickerModule::getIoken()
{
    std::string result;

    auto request=std::make_shared<Request>();
    request->queryMaker=[](){
        auto index=SteamBot::Random::generateRandomNumber()%categoryUrls.size();
        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, categoryUrls.at(index));
        return query;
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    static const std::regex regex(" data-loyalty_webapi_token=\"&quot;([a-f0-9]+)&quot;\"");
    std::smatch match;
    if (std::regex_search(html, match, regex))
    {
        assert(match.size()==2);
        result=match[1];
        BOOST_LOG_TRIVIAL(debug) << "SaleSticker: loyalty_webapi_token is \"" << result << "\"";
    }

    return result;
}

/************************************************************************/

void SaleStickerModule::run(SteamBot::Client&)
{
    waitForLogin();

    getIoken();
}

/************************************************************************/

void SteamBot::Modules::SaleSticker::use()
{
}
