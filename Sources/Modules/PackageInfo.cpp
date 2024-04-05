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

/************************************************************************/

#include "Client/Module.hpp"
#include "Helpers/URLs.hpp"
#include "Helpers/HTML.hpp"
#include "Modules/LicenseList.hpp"
#include "Modules/PackageData.hpp"
#include "Modules/PackageInfo.hpp"
#include "Modules/WebSession.hpp"
#include "HTMLParser/Parser.hpp"

#include <iostream>
#include "TypeName.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Messageboard::NewLicenses NewLicenses;

/************************************************************************/

class ErrorException { };

/************************************************************************/

namespace
{
    class PackageInfoModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<NewLicenses> newLicensesWaiter;

    private:
        void updateLicenseInfo(const SteamBot::AppID);

    public:
        void handle(std::shared_ptr<const NewLicenses>);

    public:
        PackageInfoModule() =default;
        virtual ~PackageInfoModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    PackageInfoModule::Init<PackageInfoModule> init;
}

/************************************************************************/

namespace
{
    class MainSupportPageItem
    {
    public:
        std::unique_ptr<HTMLParser::Tree::Element> date;	// <span>, "Feb 12"
        std::unique_ptr<HTMLParser::Tree::Element> text;	// <span>, "Added to your Steam library as part of: RPG Maker XP Limited Free Promotional Package - Feb 2024"

    public:
        MainSupportPageItem(decltype(date)&& date_, decltype(text)&& text_)
            : date(std::move(date_)), text(std::move(text_))
        {
        }
    };
}

/************************************************************************/

static std::vector<MainSupportPageItem> getMainSupportPage(SteamBot::AppID appId)
{
    auto request=std::make_shared<SteamBot::Modules::WebSession::Messageboard::Request>();
    request->queryMaker=[appId]() {
        static const boost::urls::url_view myUrl("https://help.steampowered.com/en/wizard/HelpWithGameIssue/?issueid=123");
        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, myUrl);
        SteamBot::URLs::setParam(query->url, "appid", SteamBot::toInteger(appId));
        return query;
    };

    std::cout << request->queryMaker()->url << std::endl;

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        throw ErrorException();
    }

    auto string=SteamBot::HTTPClient::parseString(*(response->query));

    class Parser : public HTMLParser::Parser
    {
    public:
        std::vector<MainSupportPageItem>& items;

    public:
        Parser(std::string_view html, std::vector<MainSupportPageItem>& items_)
            : HTMLParser::Parser(html), items(items_)
        {
        }

        virtual ~Parser() =default;

        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            if (element.name=="div" && SteamBot::HTML::checkClass(element, "lineitemrow"))
            {
                std::vector<std::unique_ptr<HTMLParser::Tree::Element>> spans;
                spans.reserve(2);

                for (auto& child: element.children)
                {
                    if (SteamBot::HTML::isWhitespace(*child))
                    {
                    }
                    else
                    {
                        if (auto childElement=dynamic_cast<HTMLParser::Tree::Element*>(child.get()))
                        {
                            if (childElement->name=="span")
                            {
                                if (spans.size()<2)
                                {
                                    child.release();
                                    spans.emplace_back(childElement);
                                    continue;
                                }
                            }
                        }

                        // Error: unexpected HTML
                        return;
                    }
                }

                if (spans.size()==2)
                {
                    items.emplace_back(std::move(spans[0]), std::move(spans[1]));
                }
            }
        }
    };

    std::vector<MainSupportPageItem> items;

    try
    {
        Parser(string, items).parse();
    }
    catch(...)
    {
    }

    if (items.empty())
    {
        BOOST_LOG_TRIVIAL(error) << "PackageInfo: could not find package information on main support page";
    }

    return items;
}

/************************************************************************/
/*
 * https://stackoverflow.com/a/78208586/826751
 */

void PackageInfoModule::updateLicenseInfo(const SteamBot::AppID appId)
{
    auto items=getMainSupportPage(appId);

    for (const auto& item: items)
    {
        std::cout << SteamBot::HTML::getCleanText(*item.date) << ": " << SteamBot::HTML::getCleanText(*item.text) << std::endl;
    }
}

/************************************************************************/

void PackageInfoModule::handle(std::shared_ptr<const NewLicenses> newLicenses)
{
    for (const auto packageId : newLicenses->licenses)
    {
        typedef SteamBot::Modules::LicenseList::Whiteboard::LicenseIdentifier LicenseIdentifier;
        auto packageInfo=SteamBot::Modules::PackageData::getPackageInfo(LicenseIdentifier(packageId));
        if (packageInfo!=nullptr)
        {
            for (const auto appId: packageInfo->appIds)
            {
                updateLicenseInfo(appId);
            }
        }
    }
}

/************************************************************************/

void PackageInfoModule::init(SteamBot::Client& client)
{
    newLicensesWaiter=client.messageboard.createWaiter<NewLicenses>(*waiter);
}

/************************************************************************/

void PackageInfoModule::run(SteamBot::Client&)
{
    while (true)
    {
        waiter->wait();
        newLicensesWaiter->handle(this);
    }
}

/************************************************************************/

void SteamBot::Modules::PackageInfo::use()
{
}
