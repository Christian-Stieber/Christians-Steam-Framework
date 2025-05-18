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

#include "Cloud.hpp"
#include "SafeCast.hpp"
#include "Modules/WebSession.hpp"
#include "HTMLParser/Parser.hpp"
#include "Helpers/HTML.hpp"

/************************************************************************/
/*
 * Note: there used to be a Cloud.EnumerateUserApps unified message
 * API, but it seems that was removed. I'm now downloading the cloud
 * app overview via the webpage.
 */

/************************************************************************/

typedef SteamBot::Cloud::Apps Apps;

typedef HTMLParser::Tree::Element Element;

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

Apps::~Apps() =default;
Apps::Apps() =default;

/************************************************************************/

namespace
{
    class PageParser : public HTMLParser::Parser
    {
    private:
        typedef std::function<void(const Element&)> Callback;

    private:
        Apps &apps;
        const Element* table=nullptr;

    private:
        Callback handleTable(const Element&);
        void handleHeadRow(Element&);
        void handleBodyRow(Element&);
        void handleRow(Element&);

    private:
        virtual Callback startElement(const Element&) override;
        virtual void endElement(Element&) override;

    public:
        PageParser (Apps& apps_, std::string_view html)
            : Parser (html), apps (apps_)
        {
        }

        virtual ~PageParser() =default;
    };
}

/************************************************************************/

PageParser::Callback PageParser::handleTable(const Element& element)
{
    Callback callback;
    if (table==nullptr)
    {
        if (element.name=="table" && SteamBot::HTML::checkClass(element, "accounttable"))
        {
            table=&element;
            callback=[this](const Element&) {
                table=nullptr;
            };
        }
    }
    return callback;
}

/************************************************************************/
/*
 * Expects element to have exactly one child, a Text node.
 */

static bool getTextChild(Element& element, std::string_view& result)
{
    if (element.children.size()==1)
    {
        auto text=dynamic_cast<HTMLParser::Tree::Text*>(element.children.front().get());
        if (text!=nullptr)
        {
            result=text->text;
            SteamBot::HTML::trimWhitespace(result);
            return true;
        }
    }
    return false;
}

/************************************************************************/
/*
 * Expects element to have exactly one child, an element node.
 * Everything else is ignored.
 */

static Element* getElementChild(Element& element)
{
    Element* result=nullptr;
    SteamBot::HTML::iterateChildElements(element, [&result](size_t count, Element& child)
    {
        if (count==0)
        {
            result=&child;
            return true;
        }
        else
        {
            result=nullptr;
            return false;
        }
    });
    return result;
}

/************************************************************************/

void PageParser::handleHeadRow(Element& element)
{
    static const char* headers[]={ "Game", "Number Of Files", "Total Size Of Files" };

    if (table!=nullptr)
    {
        auto elements=SteamBot::HTML::iterateChildElements(element, [](size_t count, Element& child)
        {
            if (count<=std::size(headers))
            {
                if (child.name=="th")
                {
                    std::string_view string;
                    if (getTextChild (child, string))
                    {
                        if (string==headers[count])
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        });
        if (elements!=static_cast<ssize_t>(std::size(headers)))
        {
            table=nullptr;
        }
    }
}

/************************************************************************/

static bool getGameColumn(SteamBot::Cloud::Apps::App& app, Element& td)
{
    std::string_view string;
    if (getTextChild(td, string))
    {
        app.name=std::string(string);
        return true;
    }
    return false;
}

/************************************************************************/

static bool getNumberOfFilesColumn(SteamBot::Cloud::Apps::App& app, Element& td)
{
    std::string_view string;
    if (getTextChild(td, string))
    {
        if (SteamBot::parseNumber(string, app.totalCount))
        {
            return true;
        }
    }
    return false;
}

/************************************************************************/

static bool getTotalSizeOfFilesColumn(SteamBot::Cloud::Apps::App& app, Element& td)
{
    std::string_view string;
    if (getTextChild(td, string))
    {
        double value;
        if (SteamBot::parseNumberPrefix(string, value))
        {
            if (string==" B")
            {
                app.totalSize=static_cast<decltype(app.totalSize)>(value);
                return true;
            }
            else if (string==" KB")
            {
                app.totalSize=static_cast<decltype(app.totalSize)>(value*1024);
                std::cout << app.name << " -> " << app.totalSize << "\n";
                return true;
            }
            else if (string==" MB")
            {
                app.totalSize=static_cast<decltype(app.totalSize)>(value*1024*1024);
                return true;
            }
            else if (string==" GB")
            {
                app.totalSize=static_cast<decltype(app.totalSize)>(value*1024*1024*1024);
                return true;
            }
        }
    }
    return false;
}

/************************************************************************/

static bool getShowFilesColumn(SteamBot::Cloud::Apps::App& app, Element& td)
{
    auto link=getElementChild(td);
    if (link!=nullptr)
    {
        if (link->name=="a")
        {
            std::string* href=link->getAttribute("href");
            if (href!=nullptr)
            {
                boost::urls::url_view url(*href);
                const auto &params=url.params();
                auto iterator=params.find("appid");
                if (iterator!=params.end())
                {
                    if (SteamBot::parseNumber((*iterator).value, app.appId))
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/************************************************************************/

void PageParser::handleBodyRow(Element& element)
{
    if (table!=nullptr)
    {
        SteamBot::Cloud::Apps::App app;
        auto elements=SteamBot::HTML::iterateChildElements(element, [&app](size_t count, Element& child)
        {
            if (child.name=="td")
            {
                switch(count)
                {
                case 0: return getGameColumn(app, child);
                case 1: return getNumberOfFilesColumn(app, child);
                case 2: return getTotalSizeOfFilesColumn(app, child);
                case 3: return getShowFilesColumn(app, child);
                default: break;
                }
            }
            return false;
        });
        if (elements==4)
        {
            apps.apps.emplace_back(std::move(app));
        }
        else
        {
            table=nullptr;
        }
    }
}

/************************************************************************/

void PageParser::handleRow(Element& element)
{
    if (table!=nullptr)
    {
        if (element.parent->parent==table && element.name=="tr")
        {
            if (element.parent->name=="thead")
            {
                return handleHeadRow(element);
            }
            else if (element.parent->name=="tbody")
            {
                return handleBodyRow(element);
            }
        }
    }
}

/************************************************************************/

void PageParser::endElement(Element& element)
{
    handleRow (element);
}

/************************************************************************/

PageParser::Callback PageParser::startElement(const Element& element)
{
    return handleTable(element);
}

/************************************************************************/

bool Apps::load()
{
    apps.clear();
    auto request=std::make_shared<Request>();
    request->queryMaker=[]() {
        static const boost::urls::url_view url("https://store.steampowered.com/account/remotestorage?l=english");
        return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        return false;
    }

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    PageParser parser (*this, html);
    try
    {
        parser.parse();
    }
    catch(const HTMLParser::SyntaxException&)
    {
        return false;
    }
    return true;
}

/************************************************************************/

boost::json::value Apps::App::toJson() const
{
    boost::json::object json;
    json["appId"]=SteamBot::toInteger(appId);
    json["totalCount"]=totalCount;
    json["totalSize"]=totalSize;
    return json;
}

/************************************************************************/

boost::json::value Apps::toJson() const
{
    boost::json::array json;
    for (const auto& app: apps)
    {
        json.push_back(app.toJson());
    }
    return json;
}
