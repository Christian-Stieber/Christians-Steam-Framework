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
#include "Helpers/Time.hpp"
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

#define NBSP "\xC2\xA0"

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
            unsigned int currentYear=0;
            std::unordered_map<SteamBot::PackageID, std::string> packages;
            std::vector<SteamBot::PackageID> packageIds;
            std::vector<LineItemRow> lineItemRows;

        private:
            bool handleActivation();
            bool handlePurchase();

        public:
            void handlePackageNames();
            bool validate() const;
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
        // This is the less convenient type of button on the bottom.
        // They are in a <form> that has the package-id, but there's no
        // name to be found.
        //
        // These returned in the packageIds list.

        bool handleRemovalForm(HTMLParser::Tree::Element& element)
        {
            if (element.name=="form" && SteamBot::HTML::checkAttribute(element, "id", "submit_remove_package_form"))
            {
                std::optional<SteamBot::PackageID> childPackageId;
                std::optional<SteamBot::AppID> childAppId;
                for (const auto& childNode: element.children)
                {
                    if (auto child=dynamic_cast<HTMLParser::Tree::Element*>(childNode.get()))
                    {
                        if (child->name=="input")
                        {
                            if (SteamBot::HTML::checkAttribute(*child, "id", "packageid"))
                            {
                                assert(!childPackageId);
                                childPackageId=SteamBot::HTML::getAttribute<SteamBot::PackageID>(*child, "value");
                            }
                            else if (SteamBot::HTML::checkAttribute(*child, "id", "appiid"))
                            {
                                assert(!childAppId);
                                childAppId=SteamBot::HTML::getAttribute<SteamBot::AppID>(*child, "value");
                            }
                        }
                    }
                }
                if (childPackageId && childAppId)
                {
                    assert(childAppId==appId);
                    result.packageIds.push_back(childPackageId.value());
                }
                return true;
            }
            return false;
        }

    private:
        // These are the texts at the "top" of the page, like
        //    Jan 1, 2023 -  Purchased on Steam - view receipt
        //    Nov 29, 2014 -  Activated as part of: Tomb Raider GOTY Edition
        //    Jan 7 -  Purchased as part of: Immortals Fenyx Rising - Gold Edition - view receipt
        //
        // These are returned a list of date/text HTML elements, for later processing

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
        // These are the buttons that appear when the game is provided by multiple licenses.
        // This is the jackpot -- the button itself has the name of the package, and
        // a parameter of the URL it leads to is the package-id.
        //
        // These are stored in the packages map, package-id -> name.

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
            if (!handleLineItemRow(element) && !handleRemovalForm(element))
            {
                handlePackageButton(element);
            }
        }
    };
}

/************************************************************************/
/*
 * The "delta" is added to the timestamp: I'm trying to use this to
 * account for clocks not being exactly in sync.
 */

static unsigned int getCurrentYear(std::chrono::seconds delta)
{
    struct tm myTm;
    auto currentYear=SteamBot::Time::toCalendar(std::chrono::system_clock::now()+delta, false, myTm).tm_year+1900;
    return std::make_unsigned_t<decltype(currentYear)>(currentYear);
}

/************************************************************************/

static SupportPageParser::Result getMainSupportPage(SteamBot::AppID appId)
{
    SupportPageParser::Result result;

    // Repeat loading if the year changes in between. This could mess up the
    // returned dates, since they don't include the year number if it's the
    // current one, creating a race condition.

    std::shared_ptr<const SteamBot::Modules::WebSession::Messageboard::Response> response;
    while (true)
    {
        auto request=std::make_shared<SteamBot::Modules::WebSession::Messageboard::Request>();
        request->queryMaker=[appId, &result]() {
            static const boost::urls::url_view myUrl("https://help.steampowered.com/en/wizard/HelpWithGameIssue/?issueid=123");
            auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, myUrl);
            SteamBot::URLs::setParam(query->url, "appid", SteamBot::toInteger(appId));
            result.currentYear=getCurrentYear(std::chrono::seconds(-15));
            return query;
        };

        std::cout << request->queryMaker()->url << std::endl;

        response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        if (response->query->response.result()!=boost::beast::http::status::ok)
        {
            throw ErrorException();
        }

        unsigned int currentYear=getCurrentYear(std::chrono::seconds(15));
        if (result.currentYear==currentYear)
        {
            break;
        }
        result.currentYear=currentYear;
    }
    auto string=SteamBot::HTTPClient::parseString(*(response->query));
    response.reset();

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

    assert(result.validate());
    return result;
}

/************************************************************************/
/*
 * The two formats I've seen so far are:
 *    "Sep 23, 2015 - "
 *    "Feb 12 - "
 */

namespace
{
    class ParseDate
    {
    private:
        std::string_view string;

    public:
        unsigned int day;	// 1..31
        unsigned int month;	// 0..11
        unsigned int year;	// 1900..

    private:
        bool parseString(std::string_view prefix)
        {
            if (string.starts_with(prefix))
            {
                string.remove_prefix(prefix.size());
                return true;
            }
            return false;
        }

        bool parseMonth()
        {
            static const std::string_view months[]={ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            for (month=0; month<std::size(months); month++)
            {
                if (parseString(months[month]))
                {
                    return true;
                }
            }
            return false;
        }

        bool parseInteger(unsigned int& value)
        {
            const auto result=std::from_chars(string.data(), string.data()+string.size(), value);
            if (result.ec==std::errc())
            {
                auto diff=result.ptr-string.data();
                assert(diff>=0);
                string.remove_prefix(static_cast<size_t>(diff));
                return true;
            }
            return false;
        }

        bool parseYear()
        {
            if (parseString(", "))
            {
                return parseInteger(year);
            }
            else
            {
                return true;
            }
        }

    public:
        bool parse(HTMLParser::Tree::Element& element, unsigned int currentYear)
        {
            year=currentYear;
            auto buffer=SteamBot::HTML::getCleanText(element);
            string=buffer;
            return parseMonth() && parseString(" ") && parseInteger(day) && parseYear() && parseString(NBSP "-");
        }
    };
}

/************************************************************************/
/*
 * These are some assumptions that I believe to be true, and will
 * rely on when processing the data.
 */

bool SupportPageParser::Result::validate() const
{
    if (!packages.empty())
    {
        if (!packageIds.empty()) return false;
        if (packages.size()!=lineItemRows.size()) return false;
    }
    else
    {
        if (packageIds.size()!=1) return false;
        if (lineItemRows.size()!=1) return false;
    }
    return true;
}

/************************************************************************/
/*
 * We are trying to resolve lineItemRows[0]. Check whether this is the
 *    <span>Added to your Steam library as part of: <package name></span>
 * kind, and process it if so.
 */

bool SupportPageParser::Result::handleActivation()
{
    if (lineItemRows.front().text->children.size()==1)
    {
        if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(lineItemRows.front().text->children.front().get()))
        {
            static const std::string_view prefix("Added to your Steam library as part of: ");
            if (text->text.starts_with(prefix))
            {
                std::string_view packageName(text->text);
                packageName.remove_prefix(prefix.size());
                auto success=packages.emplace(packageIds.front(), packageName).second;
                assert(success);
                return true;
            }
        }
    }
    return false;
}

/************************************************************************/
/*
 * We are trying to resolve lineItemRows[0]. Check whether this is the
 *   <span>Purchased as part of:&nbsp;<a href="...">Red Orchestra 2 - Free Day</a>&nbsp;-&nbsp;<a href="...">view receipt</a></span>
 * kind, and process it if so.
 */

bool SupportPageParser::Result::handlePurchase()
{
    if (lineItemRows.front().text->children.size()==4)
    {
        if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(lineItemRows.front().text->children[0].get()))
        {
            static const std::string_view prefix("Purchased as part of:" NBSP);
            if (text->text==prefix)
            {
                if (auto element=dynamic_cast<HTMLParser::Tree::Element*>(lineItemRows.front().text->children[1].get()))
                {
                    if (element->name=="a" && element->children.size()==1)
                    {
                        if (auto packageName=dynamic_cast<HTMLParser::Tree::Text*>(element->children.front().get()))
                        {
                            auto success=packages.emplace(packageIds.front(), std::move(packageName->text)).second;
                            assert(success);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/************************************************************************/
/*
 * This should handle the cases where the support page already tells
 * us "Actived/Purchased as part of: <package name>"
 */

void SupportPageParser::Result::handlePackageNames()
{
    if (packages.empty())
    {
        assert(packageIds.size()==1 && lineItemRows.size()==1);
        assert(lineItemRows.front().text->name=="span");
        if (handleActivation() || handlePurchase())
        {
            packageIds.clear();
        }
        assert(validate());
    }
}

/************************************************************************/
/*
 * https://stackoverflow.com/a/78208586/826751
 */

void PackageInfoModule::updateLicenseInfo(const SteamBot::AppID appId)
{
    auto result=getMainSupportPage(appId);
    result.handlePackageNames();

    if (!result.packages.empty())
    {
        for (const auto& item: result.packages)
        {
            std::cout << SteamBot::toInteger(item.first) << ": " << item.second << std::endl;
        }
    }
    else
    {
        std::cout << "unprocessed " << SteamBot::toInteger(result.packageIds.front()) << ": " << SteamBot::HTML::getCleanText(*(result.lineItemRows.front().text)) << std::endl;
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
