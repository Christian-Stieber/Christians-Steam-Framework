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

#include "HTTPClient.hpp"

#include <thread>
#include <boost/url/parse.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/fiber/future/promise.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * This is a simple HTTP client. For now, I'm assuming (or hoping...)
 * that I'll only need a few http requests, so no persistent
 * connections or anything.
 *
 * Since I want to keep the main thread "free" (for now), and we only
 * have client threads, this runs in its own thread. And because it
 * runs in its very own thread, I just use synchronous calls.
 *
 * This class is entirely internal; we only export a simple wrapper
 * function to make a query.
 *
 * Heavily based on https://www.boost.org/doc/libs/1_81_0/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp
 */

/************************************************************************/

static boost::asio::ssl::context makeSslContext()
{
    boost::asio::ssl::context sslContext{boost::asio::ssl::context::tlsv12_client};
    sslContext.set_default_verify_paths();
    sslContext.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
    return sslContext;
}

/************************************************************************/

namespace
{
    class HTTPClient
    {
    private:
        struct Params
        {
            boost::fibers::promise<SteamBot::HTTPClient::ResponseType> promise;
            boost::beast::http::verb method;
            std::string urlString;
            boost::urls::url_view url;
            std::string body;

            Params(boost::beast::http::verb method_, std::string&& urlString_, std::string&& body_)
                : method(method_), urlString(std::move(urlString_)), body(std::move(body_))
            {
                url=boost::urls::parse_absolute_uri(urlString).value();
                assert(url.scheme_id()==boost::urls::scheme::https);
            }
        };

    private:
        std::thread thread;
        boost::asio::io_context ioContext;
        boost::asio::ip::tcp::resolver resolver{ioContext};

        // ToDo: I hope its okay to share this across connections
        boost::asio::ssl::context sslContext{makeSslContext()};

    private:
        std::unique_ptr<SteamBot::HTTPClient::Response> performQuery(Params&);

    private:
        HTTPClient();
        ~HTTPClient();

        static HTTPClient& get();

    public:
        static boost::fibers::future<SteamBot::HTTPClient::ResponseType> query(boost::beast::http::verb, std::string&&, std::string&&);
    };
}

/************************************************************************/

std::unique_ptr<SteamBot::HTTPClient::Response> HTTPClient::performQuery(HTTPClient::Params& params)
{
    try
    {
        const std::string host=params.url.host();	// we need 0-termination for SSL_set_tlsext_host_name()

        std::string_view port=params.url.port();
        if (port.empty()) port="443";

        // Look up the host name
        const auto results=resolver.resolve(host, port);

        // Create the SSL stream
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioContext, sslContext);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            throw boost::beast::system_error{ec};
        }

        // Make the connection on an IP address we got from the lookup
        boost::beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        // ToDo: for some reason, certificate problems don't seem to matter...
        stream.handshake(boost::asio::ssl::stream_base::client);
#if 1
        {
            auto certificate=SSL_get_peer_certificate(stream.native_handle());
            if (certificate!=nullptr)
            {
                auto result=SSL_get_verify_result(stream.native_handle());
                BOOST_LOG_TRIVIAL(debug) << "verification result: " << result;
            }
            else
            {
                BOOST_LOG_TRIVIAL(debug) << "verification result: no certificate";
            }
        }
#endif

        {
            // Set up an HTTP request message
            boost::beast::http::request<boost::beast::http::string_body> request{params.method, params.url.encoded_target(), 11};
            request.body()=std::move(params.body);
            request.set(boost::beast::http::field::host, host);
            request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Send the HTTP request to the remote host
            boost::beast::http::write(stream, request);
        }

        // Receive the HTTP response
        std::unique_ptr<SteamBot::HTTPClient::Response> response;
        {
            auto tempResponse=std::make_unique<SteamBot::HTTPClient::Response>();
            boost::beast::http::read(stream, tempResponse->buffer, tempResponse->response);
            response=std::move(tempResponse);
        }

        // Gracefully close the stream
#if 0
        // Note: shutdown can hang: https://github.com/boostorg/beast/issues/995
        {
            beast::error_code ec;
            stream.shutdown(ec);
        }
#endif

        BOOST_LOG_TRIVIAL(info) << "HTTPClient: performQuery for \"" << params.url
                                << "\" has received a " << response->response.body().size()
                                << " byte response with code " << response->response.result();
        return response;
    }
    catch(...)
    {
        /* ToDo: add some logging */
    }

    BOOST_LOG_TRIVIAL(info) << "HTTPClient: performQuery for \"" << params.url << " has produced an error";
    throw SteamBot::HTTPClient::Exception();
}

/************************************************************************/

HTTPClient::HTTPClient()
{
    // Note: I do this here instead of the initializer list just
    // to avoid having to watch for initialization order
    thread=std::thread([this](){
        auto guard=boost::asio::make_work_guard(ioContext);
        ioContext.run();
        BOOST_LOG_TRIVIAL(info) << "HTTPClient thread terminating";
    });
}

/************************************************************************/

HTTPClient::~HTTPClient()
{
    ioContext.stop();
    thread.join();
}

/************************************************************************/

HTTPClient& HTTPClient::get()
{
    static HTTPClient* const client=new HTTPClient();
    return *client;
}

/************************************************************************/

boost::fibers::future<SteamBot::HTTPClient::ResponseType> HTTPClient::query(boost::beast::http::verb method, std::string&& url, std::string&& body)
{
    auto params=std::make_shared<Params>(method, std::move(url), std::move(body));

    HTTPClient::get().ioContext.post([params]() {
        try
        {
            params->promise.set_value(HTTPClient::get().performQuery(*params));
        }
        catch(...)
        {
            params->promise.set_exception(std::current_exception());
        }
    });
    return params->promise.get_future();
}

/************************************************************************/
/*
 * Performs a https "get" query.
 *
 * Returns a response, or throws.
 */

boost::fibers::future<SteamBot::HTTPClient::ResponseType> SteamBot::HTTPClient::query(std::string url)
{
    return ::HTTPClient::query(boost::beast::http::verb::get, std::move(url), std::string{});
}

/************************************************************************/
/*
 * Performs a https "put" query.
 *
 * Returns a response, or throws.
 */

boost::fibers::future<SteamBot::HTTPClient::ResponseType> SteamBot::HTTPClient::post(std::string url, std::string body)
{
    return ::HTTPClient::query(boost::beast::http::verb::post, std::move(url), std::move(body));
}
