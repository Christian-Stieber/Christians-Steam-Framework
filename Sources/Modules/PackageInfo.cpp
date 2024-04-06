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
/*
 * Expects one <span> child with text
 */

static std::string* getTextSpan(HTMLParser::Tree::Element& element)
{
    std::string* result=nullptr;
    for (auto& child: element.children)
    {
        if (SteamBot::HTML::isWhitespace(*child))
        {
        }
        else if (auto childElement=dynamic_cast<HTMLParser::Tree::Element*>(child.get()))
        {
            if (result==nullptr)
            {
                if (childElement->name=="span")
                {
                    if (childElement->children.size()==1)
                    {
                        if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(childElement->children.front().get()))
                        {
                            result=&text->text;
                            continue;
                        }
                    }
                }
            }
            return nullptr;
        }
        else
        {
            return nullptr;
        }
    }
    return result;
}

/************************************************************************/
/*
 * The support page I'm interested in is
 *    https://help.steampowered.com/en/wizard/HelpWithGameIssue/?appid=<APP-ID>&issueid=123
 * which is basically the page to request game removal. Don't worry, it require more
 * clicks that I'm not doing.
 *
 * There are up to two distinct sections on this page that I'm
 * interested in. In most cases, the "top" section just lists the
 * licenses that we have for the game, in various format, and not
 * always with the name of the package -- it might just be something
 * like "purchased on steam" with a link to to receipt page.
 * This might require a bit of guessing, but there's a good chance
 * that we can link package names to our package-ids by using the
 * registration date.
 *
 * If we actually own muliple packages with the game, we'll get
 * another section that lets the user select which package to remove
 * from the account. This is the jackpot case, since these buttons
 * a labelled with the package name already, and they have links with
 * the package-id.
 */

namespace
{
    class SupportPageParser : public HTMLParser::Parser
    {
    public:
        class LineItemRow
        {
        public:
            std::unique_ptr<HTMLParser::Tree::Element> date;	// <span>, "Feb 12"
            std::unique_ptr<HTMLParser::Tree::Element> text;	// <span>, "Added to your Steam library as part of: RPG Maker XP Limited Free Promotional Package - Feb 2024"

        public:
            LineItemRow(decltype(date)&& date_, decltype(text)&& text_)
                : date(std::move(date_)), text(std::move(text_))
            {
            }
        };

        class Result
        {
        public:
            std::unordered_map<SteamBot::PackageID, std::string> packages;
            std::vector<LineItemRow> lineItemRows;
        };

    private:
        SteamBot::AppID appId;
        Result& result;

    public:
        SupportPageParser(std::string_view html, SteamBot::AppID appId_, Result& result_)
            : HTMLParser::Parser(html), appId(appId_), result(result_)
        {
        }

        virtual ~SupportPageParser() =default;

    private:
        bool handleLineItemRow(HTMLParser::Tree::Element& element)
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
                        return true;
                    }
                }

                if (spans.size()==2)
                {
                    result.lineItemRows.emplace_back(std::move(spans[0]), std::move(spans[1]));
                }
                return true;
            }
            return false;
        }

    private:
        void handlePackageButton(HTMLParser::Tree::Element& element)
        {
            if (element.name=="a" &&
                SteamBot::HTML::checkClass(element, "help_wizard_button") &&
                SteamBot::HTML::checkClass(element, "help_wizard_arrow_right"))
            {
                if (auto href=element.getAttribute("href"))
                {
                    // https://help.steampowered.com/en/wizard/HelpWithGameIssue/?appid=203160&issueid=123&chosenpackage=36483
                    boost::urls::url_view url(*href);
                    if (url.path()==std::string_view("/en/wizard/helpwithgameissue/") &&
                        SteamBot::URLs::getParam<int>(url, "issueid")==123 &&
                        SteamBot::URLs::getParam<SteamBot::AppID>(url, "appid")==appId)
                    {
                        if (auto packageId=SteamBot::URLs::getParam<SteamBot::PackageID>(url, "chosenpackage"))
                        {
                            bool success=false;
                            if (auto text=getTextSpan(element))
                            {
                                success=result.packages.emplace(packageId.value(), std::move(*text)).second;
                            }
                            assert(success);
                        }
                    }
                }
            }
        }

    public:
        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            if (!handleLineItemRow(element))
            {
                handlePackageButton(element);
            }
        }
    };
}

/************************************************************************/

static SupportPageParser::Result getMainSupportPage(SteamBot::AppID appId)
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

    SupportPageParser::Result result;
    try
    {
        SupportPageParser(string, appId, result).parse();
    }
    catch(...)
    {
    }

    if (result.lineItemRows.empty())
    {
        BOOST_LOG_TRIVIAL(error) << "PackageInfo: could not find package information on main support page";
    }

    return result;
}

/************************************************************************/
/*
 * https://stackoverflow.com/a/78208586/826751
 */

void PackageInfoModule::updateLicenseInfo(const SteamBot::AppID appId)
{
    auto result=getMainSupportPage(appId);

    for (const auto& item: result.packages)
    {
        std::cout << SteamBot::toInteger(item.first) << ": " << item.second << std::endl;
    }

    for (const auto& item: result.lineItemRows)
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
                if (SteamBot::toInteger(appId)==203160)
                {
                    updateLicenseInfo(appId);
                    return;
                }
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
