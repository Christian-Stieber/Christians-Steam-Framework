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

#include "Asio/Asio.hpp"
#include "Asio/HTTPClient.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace HTTPClient
    {
        namespace Internal
        {
            class BasicQuery : public std::enable_shared_from_this<BasicQuery>
            {
            public:
                typedef std::function<void(BasicQuery&)> Callback;

            private:
                typedef boost::asio::ip::tcp::resolver Resolver;
                typedef boost::system::error_code ErrorCode;

            public:
                HTTPClient::Query* query;
                Callback callback;

            private:
                // we need 0-termination for SSL_set_tlsext_host_name()
                std::string host;

            private:
                Resolver resolver;
                std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> stream;

            private:
                static boost::asio::ssl::context& getSslContext();

            public:
                BasicQuery(HTTPClient::Query&, Callback&&);
                ~BasicQuery();

            private:
                void close();
                void complete(const ErrorCode&);

                void read_completed(const ErrorCode&, size_t);
                void write_completed(const ErrorCode&, size_t);
                void handshake_completed(const ErrorCode&);
                void connect_completed(const ErrorCode&, const Resolver::endpoint_type&);
                void resolve_completed(const ErrorCode&, Resolver::results_type);

            public:
                void perform();
            };
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace HTTPClient
    {
        namespace Internal
        {
            void performWithRedirect(std::shared_ptr<BasicQuery>);
        }
    }
}
