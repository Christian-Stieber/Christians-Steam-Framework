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
#include "Client/Mutex.hpp"
#include "Helpers/URLs.hpp"
#include "Helpers/HTML.hpp"
#include "Helpers/Time.hpp"
#include "Modules/LicenseList.hpp"
#include "Modules/PackageData.hpp"
#include "Modules/PackageInfo.hpp"
#include "Modules/WebSession.hpp"
#include "HTMLParser/Parser.hpp"
#include "UI/UI.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Messageboard::NewLicenses NewLicenses;

/************************************************************************/

class ErrorException { };

/************************************************************************/

#define NBSP "\xC2\xA0"

/************************************************************************/

static const char packageNameKey[]="name";
static const char packageUpdatedKey[]="when";

/************************************************************************/

namespace
{
    class PackageInfo
    {
    public:
        class Info
        {
        public:
            typedef std::shared_ptr<const Info> Ptr;

        public:
            std::string packageName;

        public:
            Info(decltype(packageName) packageName_)
                : packageName(std::move(packageName_))
            {
            }

            ~Info() =default;

        public:
            Info(const boost::json::value& json)
            {
                auto& object=json.as_object();
                packageName=object.at(packageNameKey).as_string();
            }

            boost::json::value toJson() const
            {
                boost::json::object json;
                json[packageNameKey]=packageName;
                return json;
            }
        };

    private:
        boost::fibers::mutex mutex;
        std::unordered_map<SteamBot::PackageID, Info::Ptr> infos;
        bool changed=false;

    private:
        SteamBot::DataFile& file{SteamBot::DataFile::get("PackageNames", SteamBot::DataFile::FileType::Steam)};
        std::chrono::steady_clock::time_point lastSave;

    private:
        PackageInfo()
        {
            file.examine([this](const boost::json::value& json) {
                for (const auto& item : json.as_object())
                {
                    SteamBot::PackageID packageId;
                    if (SteamBot::parseNumber(item.key(),packageId))
                    {
                        auto info=std::make_shared<Info>(item.value());
                        bool success=infos.emplace(packageId, std::move(info)).second;
                        assert(success);
                    }
                }
            });
            lastSave=decltype(lastSave)::clock::now();
            BOOST_LOG_TRIVIAL(debug) << "loaded package names from file: " << toJson_noMutex();
        }

        ~PackageInfo() =default;

    public:
        Info::Ptr get(SteamBot::PackageID packageId)
        {
            Info::Ptr result;
            {
                std::lock_guard<decltype(mutex)> lock(mutex);
                auto iterator=infos.find(packageId);
                if (iterator!=infos.end())
                {
                    result=iterator->second;
                }
            }
            return result;
        }

        void set(SteamBot::PackageID packageId, Info::Ptr info)
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            infos[packageId]=std::move(info);
            changed=true;
        }

    public:
        boost::json::value toJson_noMutex() const
        {
            boost::json::object json;
            for (const auto& item: infos)
            {
                char buffer[64];
                auto result=std::to_chars(buffer, buffer+sizeof(buffer)-1, SteamBot::toInteger(item.first));
                if (result.ec==std::errc())
                {
                    auto length=result.ptr-buffer;
                    assert(length>0);
                    std::string_view key(buffer, static_cast<size_t>(length));
                    bool success=json.insert(boost::json::key_value_pair(key, item.second->toJson())).second;
                    assert(success);
                }
                else
                {
                    assert(false);
                }
            }
            return json;
        }

    public:
        void save(bool force=false)
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            if (changed)
            {
                if (force || decltype(lastSave)::clock::now()-lastSave>=std::chrono::seconds(60))
                {
                    file.update([this](boost::json::value& fileData) {
                        fileData=toJson_noMutex();
                        return true;
                    });
                    lastSave=decltype(lastSave)::clock::now();
                    changed=false;
                }
            }
        }

    public:
        static PackageInfo& get()
        {
            static PackageInfo packageInfo;
            return packageInfo;
        }
    };
}

/************************************************************************/

namespace
{
    class PackageInfoModule : public SteamBot::Client::Module
    {
    private:
        // We use this to only let one fiber request a package name,
        // to avoid multiple clients requesting the same package
        // at the same time. So, this is a rather "large" mutex.
        static inline SteamBot::Mutex mutex;

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
 * In many cases, we need to get the package name from the
 * purchase receipt. We get the URL from the support page.
 */

namespace
{
    class ReceiptPageParser : public HTMLParser::Parser
    {
    public:
        class Result
        {
        public:
            std::string packageName;
        };

    private:
        Result& result;

    public:
        ReceiptPageParser(std::string_view html, Result& result_)
            : HTMLParser::Parser(html), result(result_)
        {
        }

        virtual ~ReceiptPageParser() =default;

    private:
        // <span class="purchase_detail_field">RPG Maker XP Limited Free Promotional Package - Feb 2024</span>
        bool handleName(HTMLParser::Tree::Element& element)
        {
            if (element.name=="span" && SteamBot::HTML::checkClass(element, "purchase_detail_field"))
            {
                if (element.children.size()==1)
                {
                    if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(element.children.front().get()))
                    {
                        assert(result.packageName.empty());
                        result.packageName=std::move(text->text);
                    }
                }
                return true;
            }
            return false;
        }

    public:
        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            handleName(element);
        }
    };
}

/************************************************************************/
/*
 * Checks whether we are looking at a lineItemRow referencing an inventory gift
 *
 *   Purchased as part of: <package name> - view receipt | View in your Steam Inventory
 *   In your inventory as part of: <package name> - View in your Steam Inventory
 *
 * Example case for me:
 *   https://help.steampowered.com/en/wizard/HelpWithGameIssue/?issueid=123&appid=35723
 */

static bool isInventoryLine(const HTMLParser::Tree::Element& span)
{
    assert(span.name=="span");
    if (!span.children.empty())
    {
        const auto* inventoryCandidate=dynamic_cast<const HTMLParser::Tree::Element*>(span.children.back().get());
        if (inventoryCandidate!=nullptr && inventoryCandidate->name=="span")
        {
            if (!inventoryCandidate->children.empty())
            {
                inventoryCandidate=dynamic_cast<const HTMLParser::Tree::Element*>(inventoryCandidate->children.back().get());
            }
        }
        if (inventoryCandidate!=nullptr && inventoryCandidate->name=="a" && inventoryCandidate->children.size()==1)
        {
            if (auto text=dynamic_cast<const HTMLParser::Tree::Text*>(inventoryCandidate->children.front().get()))
            {
                if (text->text=="Steam Inventory")
                {
                    BOOST_LOG_TRIVIAL(info) << "ignored an inventory line: \"" << SteamBot::HTML::getCleanText(span) << "\"";
                    return true;
                }
            }
        }
    }
    return false;
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
            boost::urls::url url;

        public:
            unsigned int currentYear=0;
            std::unordered_map<SteamBot::PackageID, std::string> packages;
            std::vector<SteamBot::PackageID> packageIds;
            std::vector<LineItemRow> lineItemRows;

        private:
            void eraseLine(size_t index)
            {
                lineItemRows.erase(lineItemRows.begin()+static_cast<decltype(lineItemRows)::difference_type>(index));
                packageIds.erase(packageIds.begin()+static_cast<decltype(packageIds)::difference_type>(index));
            }

        private:
            bool handleActivation(size_t);
            bool handlePurchase(size_t);

        public:
            void handlePackageNames();
            void getReceipts();
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
        //
        // Note: I'm now trying to ignore the lines referring to inventory gift copies, as
        // these mess up the other logic

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
                    if (!isInventoryLine(*(spans[1])))
                    {
                        result.lineItemRows.emplace_back(std::move(spans[0]), std::move(spans[1]));
                    }
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
        //
        // Note: for me, the page
        //    https://help.steampowered.com/en/wizard/HelpWithGameIssue/?appid=1078000&issueid=123
        // haas a broken button with no text; the button for the RobocraftX package somehow
        // doesn't get the name. for that reason alone I'm handling multiple
        // packageIds/lineItemRows (used to be just one): in case I encounter this empty
        // button, I just treat it as a packageId/lineItemRow case handled later.

        bool handlePackageButton(HTMLParser::Tree::Element& element)
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
                            // no button text in *my*
                            //   https://help.steampowered.com/en/wizard/HelpWithGameIssue/?appid=1078000&issueid=123
                            // It misses the "RobocraftX" package.
                            if (auto text=getTextSpan(element))
                            {
                                bool success=result.packages.emplace(packageId.value(), std::move(*text)).second;
                                assert(success);

                                // add a dummy into the packageIds
                                result.packageIds.push_back(SteamBot::PackageID::Steam);
                            }
                            else
                            {
                                // this makes us look at the lineItemRow later, to
                                // get the name from it or load the receipt page
                                result.packageIds.push_back(packageId.value());
                            }
                            return true;
                        }
                    }
                }
            }
            return false;
        }

    public:
        virtual void endElement(HTMLParser::Tree::Element& element) override
        {
            handleLineItemRow(element) || handleRemovalForm(element) || handlePackageButton(element);
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

        result.url=request->queryMaker()->url;

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

    return result;
}

#if 0
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
#endif

/************************************************************************/
/*
 * We are trying to resolve lineItemRows[0]. Check whether this is the
 *    <span>Added to your Steam library as part of: <package name></span> or
 *    <span>Activated as part of: <package name></span> or
 * kind, and process it if so.
 *
 * "index" is for the lineItemRows/packageIds.
 */

bool SupportPageParser::Result::handleActivation(size_t index)
{
    const auto& lineItemRow=lineItemRows.at(index);
    if (lineItemRow.text->children.size()==1)
    {
        if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(lineItemRow.text->children.front().get()))
        {
            static const std::string_view prefixes[]={
                "Added to your Steam library as part of:",
                "Activated as part of:"
            };

            for (const auto& prefix: prefixes)
            {
                if (text->text.starts_with(prefix))
                {
                    std::string_view packageName(text->text);
                    packageName.remove_prefix(prefix.size());
                    if (packageName.starts_with(" "))
                    {
                        packageName.remove_prefix(1);
                    }
                    else if (packageName.starts_with(NBSP))
                    {
                        packageName.remove_prefix(strlen(NBSP));
                    }
                    auto success=packages.emplace(packageIds.at(index), packageName).second;
                    assert(success);
                    return true;
                }
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
 *
 * "index" is for the lineItemRows/packageIds.
*/

bool SupportPageParser::Result::handlePurchase(size_t index)
{
    const auto& lineItemRow=lineItemRows.at(index);
    if (lineItemRow.text->children.size()==4)
    {
        if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(lineItemRow.text->children[0].get()))
        {
            static const std::string_view prefix("Purchased as part of:" NBSP);
            if (text->text==prefix)
            {
                if (auto element=dynamic_cast<HTMLParser::Tree::Element*>(lineItemRow.text->children[1].get()))
                {
                    if (element->name=="a" && element->children.size()==1)
                    {
                        if (auto packageName=dynamic_cast<HTMLParser::Tree::Text*>(element->children.front().get()))
                        {
                            auto success=packages.emplace(packageIds.at(index), std::move(packageName->text)).second;
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
 *
 * Erases the successfully processed lineItemRow/packageId from the data.
 */

void SupportPageParser::Result::handlePackageNames()
{
    if (lineItemRows.size()==packageIds.size())
    {
        size_t index=0;
        while (index<lineItemRows.size())
        {
            assert(lineItemRows.at(index).text->name=="span");

            if (packageIds.at(index)==SteamBot::PackageID::Steam ||
                handleActivation(index) ||
                handlePurchase(index))
            {
                eraseLine(index);
            }
            else
            {
                index++;
            }
        }
    }
}

/************************************************************************/
/*
 * Not actually the receipt page, but it seems to work anyway.
 * See transformReceiptLink() below.
 */

static ReceiptPageParser::Result getReceipt(const boost::urls::url_view_base& url)
{
    ReceiptPageParser::Result result;

    auto request=std::make_shared<SteamBot::Modules::WebSession::Messageboard::Request>();
    request->queryMaker=[&url]() {
        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        return query;
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        throw ErrorException();
    }

    auto string=SteamBot::HTTPClient::parseString(*(response->query));

    try
    {
        ReceiptPageParser(string, result).parse();
    }
    catch(...)
    {
    }

    return result;
}

/************************************************************************/
/*
 * For some reason, I couldn't figure out how to load the actual
 * receipt page (i.e. the one that "view receipt" points to) --
 * no matter what, I always end up at a generic support page.
 *
 * However, I can "ask" a more specific question to get to another
 * page which also has the package name... and it loads just fine.
 * Funny enough, it even works with the receipt page parser that
 * I made initially...
 */

static bool transformReceiptLink(const std::string& href, boost::urls::url& url)
{
    const boost::urls::url_view hrefUrl(href);
    auto transactionId=hrefUrl.params().find("transid");
    if (transactionId!=hrefUrl.params().end())
    {
        static const boost::urls::url_view newUrl{"https://help.steampowered.com/en/wizard/HelpWithPurchaseIssue/?issueid=213"};
        url=newUrl;
        url.params().set((*transactionId).key, (*transactionId).value);
        return true;
    }
    return false;
}

/************************************************************************/
/*
 * This is meant to handle lineItemRows of the kind
 *    <span>Purchased on Steam&nbsp;-&nbsp;<a href="...">view receipt</a></span>
 */

void SupportPageParser::Result::getReceipts()
{
    if (lineItemRows.size()==packageIds.size())
    {
        for (size_t index=0; index<lineItemRows.size(); index++)
        {
            const auto& lineItemRow=lineItemRows.at(index);

            assert(lineItemRow.text->name=="span");
            if (lineItemRow.text->children.size()==2)
            {
                if (auto text=dynamic_cast<HTMLParser::Tree::Text*>(lineItemRow.text->children[0].get()))
                {
                    static const std::string_view prefix("Purchased on Steam" NBSP "-" NBSP);
                    if (text->text==prefix)
                    {
                        if (auto element=dynamic_cast<HTMLParser::Tree::Element*>(lineItemRow.text->children[1].get()))
                        {
                            if (element->name=="a" && element->children.size()==1)
                            {
                                if (auto viewText=dynamic_cast<HTMLParser::Tree::Text*>(element->children.front().get()))
                                {
                                    static const std::string_view viewReceipt("view receipt");
                                    if (viewText->text==viewReceipt)
                                    {
                                        if (auto href=element->getAttribute("href"))
                                        {
                                            boost::urls::url myUrl;
                                            if (transformReceiptLink(*href, myUrl))
                                            {
                                                const boost::urls::url_view receiptUrl(*href);
                                                auto result=getReceipt(myUrl);
                                                if (!result.packageName.empty())
                                                {
                                                    bool success=packages.emplace(packageIds.at(index), std::move(result.packageName)).second;
                                                    assert(success);

                                                    eraseLine(index);
                                                    index--;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
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
    result.getReceipts();

    if (!result.packages.empty())
    {
        for (auto& item: result.packages)
        {
            BOOST_LOG_TRIVIAL(info) << "app-id " << SteamBot::toInteger(appId) << "; package-id " << SteamBot::toInteger(item.first) << " has name \"" << item.second << "\"";
            SteamBot::UI::OutputText() << "app-id " << SteamBot::toInteger(appId) << "; package-id " << SteamBot::toInteger(item.first) << " has name \"" << item.second << "\"";
            PackageInfo::get().set(item.first, std::make_shared<PackageInfo::Info>(std::move(item.second)));
        }
    }
    else
    {
        BOOST_LOG_TRIVIAL(info)
            << "app-id" << SteamBot::toInteger(appId)
            << ": unable to find name for package-id "
            << SteamBot::toInteger(result.packageIds.front()) << ": "
            << SteamBot::HTML::getCleanText(*(result.lineItemRows.front().text));
        SteamBot::UI::OutputText() << "app-id " << SteamBot::toInteger(appId) << ": unable to find name for package-id " << SteamBot::toInteger(result.packageIds.front());
    }
}

/************************************************************************/

void PackageInfoModule::handle(std::shared_ptr<const NewLicenses> newLicenses)
{
    for (const auto packageId : newLicenses->licenses)
    {
        auto cancellation=SteamBot::Client::getClient().cancel.registerObject(mutex);

        std::lock_guard<decltype(mutex)> lock(mutex);
        if (auto info=PackageInfo::get().get(packageId))
        {
            BOOST_LOG_TRIVIAL(info) << "package-id " << SteamBot::toInteger(packageId) << " already has known name \"" << info->packageName << "\"";
        }
        else
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
            PackageInfo::get().save(false);
        }
    }
    PackageInfo::get().save(true);
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
