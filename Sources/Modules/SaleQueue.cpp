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

#include "Modules/WebSession.hpp"
#include "Modules/SaleQueue.hpp"
#include "Modules/DiscoveryQueue.hpp"
#include "Helpers/HTML.hpp"
#include "UI/UI.hpp"
#include "Client/Sleep.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/url/url_view.hpp>

#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    enum class QueueState { Error, NoCards, HasCards };

    static const int maxErrors=10;
}

/************************************************************************/
/*
 * Again, inspired by ASF
 */

static QueueState hasSaleCards()
{
    BOOST_LOG_TRIVIAL(debug) << "SaleQueue.cpp: hasSaleCards()";

    class QueuePageParser : public HTMLParser::Parser
    {
    public:
        QueueState hasCards=QueueState::Error;

    public:
        QueuePageParser(std::string_view html)
            : Parser(html)
        {
        }

        virtual ~QueuePageParser() =default;

    private:
        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            if (element.name=="div" && SteamBot::HTML::checkClass(element, "subtext"))
            {
                auto text=SteamBot::HTML::getCleanText(element);
                BOOST_LOG_TRIVIAL(debug) << "queue page: found text \"" << text << "\"";
                SteamBot::UI::OutputText() << "queue status: \"" << text << "\"";
                if (text.starts_with("You can get "))
                {
                    hasCards=QueueState::HasCards;
                }
                else
                {
                    hasCards=QueueState::NoCards;
                }
            }
        }
    };

    auto request=std::make_shared<Request>();
    request->queryMaker=[](){
        static const boost::urls::url_view url("https://store.steampowered.com/explore?l=english");
        return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
    };
    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
    if (response->query->response.result()!=boost::beast::http::status::ok)
    {
        return QueueState::Error;
    }

    auto html=SteamBot::HTTPClient::parseString(*(response->query));
    QueuePageParser parser(html);
    parser.parse();

    return parser.hasCards;
}

/************************************************************************/
/*
 * ToDo: what exactly does the bool result indicate?
 */

static bool performClear()
{
    BOOST_LOG_TRIVIAL(debug) << "SaleQueue.cpp: performClear()";
    try
    {
        int errorCount=0;
        while (true)
        {
            bool error=true;
            switch(hasSaleCards())
            {
            case QueueState::Error:
                break;

            case QueueState::NoCards:
                SteamBot::UI::OutputText() << "no sale queue to be cleared";
                return true;

            case QueueState::HasCards:
                if (SteamBot::DiscoveryQueue::clear())
                {
                    error=false;
                }
                break;

            default:
                assert(false);
            }

            if (error)
            {
                SteamBot::UI::OutputText() << "sale queue had an error";
                errorCount++;
                if (errorCount>=maxErrors)
                {
                    SteamBot::UI::OutputText() << "sale queue got too many errors; giving up";
                    return false;
                }
            }

            SteamBot::sleep(std::chrono::seconds(30));
        }
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "SaleQueue: exception " << boost::current_exception_diagnostic_information();
    }
    return false;
}

/************************************************************************/

bool SteamBot::SaleQueue::clear()
{
    static thread_local boost::fibers::mutex mutex;
    static thread_local bool lastResult=false;

    std::unique_lock<decltype(mutex)> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        lock.lock();
        return lastResult;
    }

    return lastResult=performClear();
}
