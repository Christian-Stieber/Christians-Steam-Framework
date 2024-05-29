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
#include "Modules/OwnedGames.hpp"
#include "Modules/BadgeData.hpp"
#include "Modules/InventoryNotification.hpp"
#include "Modules/CardFarmer.hpp"
#include "Helpers/URLs.hpp"
#include "Helpers/HTML.hpp"
#include "Helpers/NumberString.hpp"
#include "UI/UI.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData BadgeData;
typedef SteamBot::Modules::BadgeData::Messageboard::UpdateBadge UpdateBadge;

typedef SteamBot::Modules::InventoryNotification::Messageboard::InventoryNotification InventoryNotification;

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
typedef SteamBot::Modules::OwnedGames::Messageboard::GameChanged GameChanged;

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

typedef SteamBot::Modules::CardFarmer::Settings::Enable Enable;

/************************************************************************/

BadgeData::BadgeData() =default;
BadgeData::~BadgeData() =default;

/************************************************************************/

namespace
{
    class BadgePageParser : public HTMLParser::Parser
    {
    public:
        BadgeData& data;
        bool hasNextPage=false;		// true, if we found a next-page button

    public:
        BadgePageParser(std::string_view html, BadgeData& data_)
            : Parser(html), data(data_)
        {
        }

        virtual ~BadgePageParser() =default;

    private:
        bool handle_data(const HTMLParser::Tree::Element& element)
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

    private:
        bool handle_pagebtn(const HTMLParser::Tree::Element& element)
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

    private:
        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            handle_data(element) || handle_pagebtn(element);
        }
    };
}

/************************************************************************/

namespace
{
    class GetBadgeDataModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<OwnedGames::Ptr> ownedGamesWaiter;
        SteamBot::Messageboard::WaiterType<UpdateBadge> updateBadgeWaiter;
        SteamBot::Messageboard::WaiterType<InventoryNotification> inventoryNotificationWaiter;
        SteamBot::Messageboard::WaiterType<GameChanged> gameChangedWaiter;
        SteamBot::Whiteboard::WaiterType<Enable::Ptr<Enable>> enableWaiter;

    private:
        void updateBadge(SteamBot::AppID);
        void getOverviewPages();

        static bool getURL(boost::urls::url, BadgeData&, bool);

    public:
        void handle(std::shared_ptr<const UpdateBadge>);
        void handle(std::shared_ptr<const InventoryNotification>);
        void handle(std::shared_ptr<const GameChanged>);

    public:
        GetBadgeDataModule() =default;
        virtual ~GetBadgeDataModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    GetBadgeDataModule::Init<GetBadgeDataModule> init;
}

/************************************************************************/

void GetBadgeDataModule::init(SteamBot::Client& client)
{
    ownedGamesWaiter=client.whiteboard.createWaiter<OwnedGames::Ptr>(*waiter);
    updateBadgeWaiter=client.messageboard.createWaiter<UpdateBadge>(*waiter);
    inventoryNotificationWaiter=client.messageboard.createWaiter<InventoryNotification>(*waiter);
    gameChangedWaiter=client.messageboard.createWaiter<GameChanged>(*waiter);
    enableWaiter=client.whiteboard.createWaiter<Enable::Ptr<Enable>>(*waiter);
}

/************************************************************************/
/*
 * The HTML between the badge overview page and the pages for
 + specific games are so similar (at least concerning the information
 * that we want), we can use the same parser.
 *
 * So, this loads either URL and returns the badget information
 * found on the page.
 *
 * Returns "true" if the page had a "netx page" button.
 *
 * For now, we don't really communicate errors. So, we either have
 * data, or we don't.
 */

bool GetBadgeDataModule::getURL(boost::urls::url url, BadgeData& badgeData, bool detailPage)
{
    auto request=std::make_shared<Request>();
    request->queryMaker=[&url]() {
        return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, std::move(url));
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        return false;
    }

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    BadgePageParser parser(html, badgeData);
    try
    {
        parser.parse();
    }
    catch(const HTMLParser::SyntaxException&)
    {
        // Unfortunately, the detail page has an unclosed element on it...
        if (!detailPage) throw;
    }
    return parser.hasNextPage;
}

/************************************************************************/
/*
 * This loeads all the badge overview pages
 */

void GetBadgeDataModule::getOverviewPages()
{
    auto badgeData=std::make_shared<BadgeData>();
    unsigned int page=1;

    while (true)
    {
        SteamBot::UI::OutputText() << "BadgeData: loading overview page " << page;

        auto url=SteamBot::URLs::getClientCommunityURL();
        url.segments().push_back("badges");
        if (page>1)
        {
            url.params().set("p", SteamBot::toString(page));
        }

        if (!getURL(std::move(url), *badgeData, false))
        {
            SteamBot::UI::OutputText() << "BadgeData: got data for " << badgeData->badges.size() << " games";
            getClient().whiteboard.set<BadgeData::Ptr>(std::move(badgeData));
            break;
        }

        page++;
    }
}

/************************************************************************/
/*
 * ToDo: the organization of the badge data is... unfortunate.
 *
 * We need to copy the entire thing to make updates, and we can't even
 * skip this if the "updated" item is still the same because of the
 * timestamp.
 *
 * ToDo: also, we might want to think about delaying update for a
 * short while, in case we get more requests (like from receiving a
 * bunch of cards in one go).
 */

void GetBadgeDataModule::updateBadge(SteamBot::AppID appId)
{
    auto& whiteboard=getClient().whiteboard;
    if (auto existing=whiteboard.get<BadgeData::Ptr>())
    {
        BadgeData badgeData;

        auto url=SteamBot::URLs::getClientCommunityURL();
        url.segments().push_back("gamecards");
        url.segments().push_back(SteamBot::toString(SteamBot::toInteger(appId)));

        SteamBot::UI::OutputText() << "BadgeData: loading page for app " << appId;

        getURL(std::move(url), badgeData, true);
        if (badgeData.badges.size()==1)
        {
            auto newData=std::make_shared<BadgeData>(*existing);

            auto newItemIterator=badgeData.badges.cbegin();
            SteamBot::UI::OutputText() << "BadgeData: "
                                       << newItemIterator->first << " has "
                                       << newItemIterator->second.cardsEarned << " cards earned, "
                                       << newItemIterator->second.cardsReceived << " received";
            newData->badges.insert_or_assign(newItemIterator->first, newItemIterator->second);
            whiteboard.set<BadgeData::Ptr>(std::move(newData));
        }
        else
        {
            SteamBot::UI::OutputText() << "BadgeData: no data found for " << appId;
        }
    }
}

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const InventoryNotification> message)
{
    BOOST_LOG_TRIVIAL(info) << "BadgeData: received an inventory notification: " << message->toJson();

    if (message->assetInfo->itemType==SteamBot::AssetData::AssetInfo::ItemType::TradingCard)
    {
        if (auto badgeData=getClient().whiteboard.has<BadgeData::Ptr>())
        {
            const auto& badges=(*badgeData)->badges;
            auto badgeInfo=badges.find(message->assetInfo->marketFeeApp);
            if (badgeInfo!=badges.cend())
            {
                if (badgeInfo->second.cardsEarned>badgeInfo->second.cardsReceived)
                {
                    updateBadge(message->assetInfo->marketFeeApp);
                }
            }
        }
    }
}

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const UpdateBadge> message)
{
    updateBadge(message->appId);
}

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const GameChanged> message)
{
    if (message->newGame)
    {
        updateBadge(message->appId);
    }
}

/************************************************************************/

void GetBadgeDataModule::run(SteamBot::Client& client)
{
    bool fullyLoaded=false;

    while (true)
    {
        waiter->wait();

        if (SteamBot::Settings::getValue<Enable>(enableWaiter))
        {
            // Note: for now, I'm trying to stick to GameChanged
            // messages for game additions, instead of reading the
            // whole thing again.

            if (!fullyLoaded)
            {
                if (ownedGamesWaiter->has())
                {
                    getOverviewPages();
                    fullyLoaded=true;
                }
            }
            else
            {
                ownedGamesWaiter->has();
            }

            updateBadgeWaiter->handle(this);
            inventoryNotificationWaiter->handle(this);
            gameChangedWaiter->handle(this);
        }
        else
        {
            updateBadgeWaiter->discardMessages();
            inventoryNotificationWaiter->discardMessages();
            gameChangedWaiter->discardMessages();
            client.whiteboard.clear<BadgeData::Ptr>();
            fullyLoaded=false;
        }
    }
}

/************************************************************************/

void SteamBot::Modules::BadgeData::Messageboard::UpdateBadge::update(AppID appId)
{
    auto message=std::make_shared<UpdateBadge>(appId);
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}

/************************************************************************/

void SteamBot::Modules::BadgeData::use()
{
}
