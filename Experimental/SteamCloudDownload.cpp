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

#include "Modules/UnifiedMessageClient.hpp"
#include "Client/Module.hpp"
#include "Modules/TestModule.hpp"
#include "Asio/HTTPClient.hpp"
#include "UI/UI.hpp"

#include "steamdatabase/protobufs/steam/steammessages_cloud.steamclient.pb.h"

#include <boost/url/url.hpp>

/************************************************************************/
/*
 * Note: add these proto files to the CMakeLists:
 *   list(APPEND protoFiles steam/steammessages_cloud.steamclient)
 *   list(APPEND protoFiles steam/steammessages_client_objects)
 */

/************************************************************************/

namespace
{
    class TestModule : public SteamBot::Client::Module
    {
    public:
        TestModule() =default;
        virtual ~TestModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    TestModule::Init<TestModule> init;
}

/************************************************************************/

void TestModule::init(SteamBot::Client&)
{
}

/************************************************************************/
/*
 * "Proof of concept": download a file from the Steam-Cloud
 */

void TestModule::run(SteamBot::Client&)
{
    waitForLogin();

    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Cloud::ClientFileDownload)> ClientFileDownloadInfo;
    std::shared_ptr<ClientFileDownloadInfo::ResultType> response;
    {
        ClientFileDownloadInfo::RequestType request;
        request.set_appid(7);
        request.set_filename("sharedconfig.vdf");
        request.set_realm(1);
        response=SteamBot::Modules::UnifiedMessageClient::execute<ClientFileDownloadInfo::ResultType>("Cloud.ClientFileDownload#1", std::move(request));
    }

    std::string url(response->use_https() ? "https://" : "http://");
    url+=response->url_host();
    url+=response->url_path();
    auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, boost::urls::url(url));
#if 1
    query=SteamBot::HTTPClient::perform(std::move(query));
    auto data=SteamBot::HTTPClient::parseString(*query);
    SteamBot::UI::OutputText() << data;
#else
    SteamBot::UI::OutputText() << query->url;
#endif

    while (true)
    {
        waiter->wait();
    }
}

/************************************************************************/

void SteamBot::Modules::TestModule::use()
{
}
