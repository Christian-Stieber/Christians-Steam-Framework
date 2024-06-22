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

#include "./BasicQuery.hpp"

#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

static bool checkRedirect(SteamBot::HTTPClient::Query& query)
{
    auto status=query.response.result();
    if (status==boost::beast::http::status::found)
    {
        auto location=query.response.base()["Location"];
        if (!location.empty())
        {
            BOOST_LOG_TRIVIAL(info) << "we've been redirected from \"" << query.url << "\" to \"" << location << "\"";
            try
            {
                boost::urls::url redirected(location);
                if (query.url!=redirected)
                {
                    query.url=std::move(redirected);
                    return true;
                }
                BOOST_LOG_TRIVIAL(error) << "redirection loop on " << query.url;
            }
            catch(...)
            {
                BOOST_LOG_TRIVIAL(info) << "server returned invalid redirection URL: \"" << location << "\": " << boost::current_exception_diagnostic_information();
            }
        }
    }
    return false;
}

/************************************************************************/
/*
 * This executes a BasicQuery, and handles "redirect" status code in
 * the reponse.
 */

void SteamBot::HTTPClient::Internal::performWithRedirect(std::shared_ptr<BasicQuery> query)
{
    query->callback=[originalCallback=std::move(query->callback)](BasicQuery& basicQuery) {
        if (checkRedirect(*(basicQuery.query)))
        {
            SteamBot::Asio::getIoContext().post([query=basicQuery.shared_from_this()]() mutable {
                performWithRedirect(std::move(query));
            });
        }
        else
        {
            originalCallback(basicQuery);
        }
    };
    query->perform();
}
