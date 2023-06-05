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
#include "Modules/Login.hpp"
#include "Modules/GetBadgeData.hpp"
#include "Helpers/URLs.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::GetURL GetURL;
typedef SteamBot::Modules::WebSession::Messageboard::GotURL GotURL;

/************************************************************************/

namespace
{
    class GetBadgeDataModule : public SteamBot::Client::Module
    {
    public:
        void handle(std::shared_ptr<const GotURL>);

    public:
        GetBadgeDataModule() =default;
        virtual ~GetBadgeDataModule() =default;

        virtual void run() override;
    };

    GetBadgeDataModule::Init<GetBadgeDataModule> init;
}

/************************************************************************/

void GetBadgeDataModule::handle(std::shared_ptr<const GotURL> message)
{
    typedef SteamBot::Modules::GetPageData::Whiteboard::BadgePageData BadgePageData;
    BadgePageData data(SteamBot::HTTPClient::parseString(*(message->response)));
    BOOST_LOG_TRIVIAL(debug) << "Got badge page data: " << data;
}

/************************************************************************/

void GetBadgeDataModule::run()
{
    getClient().launchFiber("GetBadgeDataModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();
        auto cancellation=getClient().cancel.registerObject(*waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter->createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        std::shared_ptr<SteamBot::Messageboard::Waiter<GotURL>> gotUrl;
        gotUrl=waiter->createWaiter<decltype(gotUrl)::element_type>(getClient().messageboard);

        std::shared_ptr<GetURL> getUrl;

        while (true)
        {
            waiter->wait();

            gotUrl->handle(this);

            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                if (!getUrl)
                {
                    getUrl=std::make_shared<GetURL>();
                    getUrl->url=SteamBot::URLs::getClientCommunityURL();
                    getUrl->url.segments().push_back("badges");

                    getClient().messageboard.send(getUrl);
                }
            }
        }
    });
}
