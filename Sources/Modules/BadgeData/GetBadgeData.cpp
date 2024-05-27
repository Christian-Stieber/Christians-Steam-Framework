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
#include "Helpers/URLs.hpp"
#include "UI/UI.hpp"

#include "./Header.hpp"

#include <charconv>

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData BadgeData;
typedef SteamBot::Modules::BadgeData::Messageboard::UpdateBadge UpdateBadge;

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

BadgeData::BadgeData() =default;
BadgeData::~BadgeData() =default;

/************************************************************************/

namespace
{
    class GetBadgeDataModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<OwnedGames::Ptr> ownedGamesWaiter;
        SteamBot::Messageboard::WaiterType<UpdateBadge> updateBadgeWaiter;

    private:
        bool getAll();
        void updateBadges();

    public:
        void handle(std::shared_ptr<const UpdateBadge>);

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
}

/************************************************************************/
/*
 * Load all pages at https://steamcommunity.com/.../badges/
 */

bool GetBadgeDataModule::getAll()
{
    auto badgeData=std::make_shared<BadgeData>();
    unsigned int page=1;

    while (true)
    {
        SteamBot::UI::OutputText() << "loading badge page " << page;
        auto request=std::make_shared<Request>();
        request->queryMaker=[page](){
            auto url=SteamBot::URLs::getClientCommunityURL();
            url.segments().push_back("badges");
            if (page>1)
            {
                char string[16];
                auto result=std::to_chars(string, string+sizeof(string), page);
                assert(result.ec==std::errc());
                url.params().set("p", std::string_view(string, result.ptr));
            }
            return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        };

        auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        if (response->query->response.result()!=boost::beast::http::status::ok)
        {
            return false;
        }

        auto html=SteamBot::HTTPClient::parseString(*(response->query));

        if (!SteamBot::GetPageData::parseBadgePage(html, *badgeData))
        {
            SteamBot::UI::OutputText() << "got badge data for " << badgeData->badges.size() << " games";
            getClient().whiteboard.set<BadgeData::Ptr>(std::move(badgeData));
            return true;
        }

        page++;
    }
}

/************************************************************************/

void GetBadgeDataModule::updateBadges()
{
    ownedGamesWaiter->has();

    if (!getClient().whiteboard.has<BadgeData::Ptr>())
    {
        getAll();
    }
}

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const UpdateBadge>)
{
}

/************************************************************************/

void GetBadgeDataModule::run(SteamBot::Client&)
{
    while (true)
    {
        waiter->wait();
        if (ownedGamesWaiter->isWoken())
        {
            updateBadges();
        }

        updateBadgeWaiter->handle(this);
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
