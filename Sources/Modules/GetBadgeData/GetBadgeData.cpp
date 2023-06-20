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
#include "Modules/GetBadgeData.hpp"
#include "Helpers/URLs.hpp"
#include "UI/UI.hpp"

#include "./Header.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::GetURL GetURL;
typedef SteamBot::Modules::WebSession::Messageboard::GotURL GotURL;

typedef SteamBot::Modules::GetPageData::Whiteboard::BadgeData BadgeData;

/************************************************************************/

BadgeData::BadgeData() =default;
BadgeData::~BadgeData() =default;

/************************************************************************/

namespace
{
    class GetBadgeDataModule : public SteamBot::Client::Module
    {
    public:
        class ChainLoader;
        std::unique_ptr<ChainLoader> loader;

    public:
        void handle(std::shared_ptr<const GotURL>);

    private:
        void requestNextPage();

    public:
        GetBadgeDataModule() =default;
        virtual ~GetBadgeDataModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    GetBadgeDataModule::Init<GetBadgeDataModule> init;
}

/************************************************************************/

class GetBadgeDataModule::ChainLoader
{
public:
    boost::urls::url currentUrl;
    BadgeData::Ptr collectedData;
    std::shared_ptr<GetURL> currentQuery;

public:
    ChainLoader()
        : currentUrl(SteamBot::URLs::getClientCommunityURL()),
          collectedData(std::make_shared<BadgeData>())
    {
        currentUrl.segments().push_back("badges");
    }

public:
    void loadPage()
    {
        currentQuery=std::make_shared<GetURL>();
        currentQuery->url=currentUrl;
        getClient().messageboard.send(currentQuery);
    }
};

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const GotURL> message)
{
    if (!loader || message->initiator!=loader->currentQuery)
    {
        return;
    }

    if (!message->query->redirectedUrl.empty())
    {
        loader->currentUrl=message->query->redirectedUrl;
    }

    auto html=SteamBot::HTTPClient::parseString(*(message->query));
#if 0
    BOOST_LOG_TRIVIAL(debug) << html;
#endif

    auto nextPage=SteamBot::GetPageData::parseBadgePage(html, *(loader->collectedData));

    if (nextPage.empty())
    {
        BOOST_LOG_TRIVIAL(debug) << "badge data: " << *(loader->collectedData);
        SteamBot::UI::OutputText() << "got " << loader->collectedData->badges.size() << " records of badge data";
        getClient().whiteboard.set(std::move(loader->collectedData));
        loader.reset();
    }
    else
    {
        assert(nextPage[0]=='?');
        loader->currentUrl.set_encoded_query(std::string_view(nextPage.data()+1, nextPage.size()-1));
        requestNextPage();
    }
}

/************************************************************************/

void GetBadgeDataModule::requestNextPage()
{
    if (!loader)
    {
        loader=std::make_unique<ChainLoader>();
    }
    loader->loadPage();
}

/************************************************************************/

void GetBadgeDataModule::run(SteamBot::Client& client)
{
    waitForLogin();

    std::shared_ptr<SteamBot::Messageboard::Waiter<GotURL>> gotUrl;
    gotUrl=waiter->createWaiter<decltype(gotUrl)::element_type>(client.messageboard);

    requestNextPage();

    while (true)
    {
        waiter->wait();
        gotUrl->handle(this);
    }
}
