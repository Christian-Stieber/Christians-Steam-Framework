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

#pragma once

#include "Client/ResultWaiter.hpp"

#include <string_view>
#include <boost/url/url.hpp>
#include <boost/json.hpp>

/************************************************************************/
/*
 * This models a standard Steam WebAPI request.
 *
 * See https://partner.steamgames.com/doc/webapi_overview for an general
 * overview on these requests.
 *
 * For now, we have name->string and name->array-of-string parameters.
 * The latter has "count" hardcoded as per the website; if that turns
 * out to be bad information I'll add that name as an additional
 * argument.
 *
 * While I haven't seen an actual "responses are in JSON", I'll
 * assume they are... at least for now. Thus, all reponses will
 * be decoded for you.
 */

/************************************************************************/

namespace SteamBot
{
    namespace WebAPI
    {
        class Query
        {
        public:
            boost::urls::url url;

        public:
            Query(std::string_view, std::string_view, unsigned int);
            virtual ~Query();

        public:
            // Result data will be filled in during perform()
            boost::system::error_code error;
            boost::json::value value;

            typedef std::unique_ptr<Query> QueryPtr;
            typedef SteamBot::ResultWaiter<QueryPtr> WaiterType;

        public:
            void set(std::string_view, std::string_view);
            void set(std::string_view, const std::vector<std::string>&);

            void set(std::string_view, int);	// for now, let's just assume this is enough
        };

        std::shared_ptr<Query::WaiterType> perform(std::shared_ptr<SteamBot::WaiterBase>, Query::QueryPtr);
    }
}
