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
#include <boost/json/stream_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>

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
#ifndef _WIN32
    sslContext.set_default_verify_paths();
    sslContext.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
#else
    // ToDo: Windows-openssl doesn't seem to have certificates?
    sslContext.set_verify_mode(boost::asio::ssl::verify_none);
#endif
    return sslContext;
}

/************************************************************************/
/*
 * https://stackoverflow.com/questions/9828066/how-to-decipher-a-boost-asio-ssl-error-code
 */

#include <openssl/err.h>
#include <boost/lexical_cast.hpp>

static void printSslError(const boost::system::error_code& error)
{
    std::string err = error.message();
    if (error.category() == boost::asio::error::get_ssl_category()) {
        err = std::string(" (")
            +boost::lexical_cast<std::string>(ERR_GET_LIB(error.value()))+","
            //+boost::lexical_cast<std::string>(ERR_GET_FUNC(error.value()))+","
            +boost::lexical_cast<std::string>(ERR_GET_REASON(error.value()))+") "
            ;
        //ERR_PACK /* crypto/err/err.h */
        char buf[128];
        ::ERR_error_string_n(error.value(), buf, sizeof(buf));
        err += buf;
        BOOST_LOG_TRIVIAL(error) << "ssl error: " << err;
    }
}

/************************************************************************/

namespace
{
    class HTTPClient
    {
    private:
        // ToDo: move the "promise" into the RequestType, so we can
        // access it from the user side to cancel a wait?
        struct Params
        {
            boost::fibers::promise<SteamBot::HTTPClient::ResponseType> promise;
            SteamBot::HTTPClient::RequestType request;
        };

    private:
        struct RedirectedURL
        {
            std::string buffer;
            boost::urls::url_view url;

            RedirectedURL() =default;

            RedirectedURL(std::string_view location)
                : buffer(location),
                  url(boost::urls::parse_absolute_uri(buffer).value())
            {
            }
        };

    private:
        std::thread thread;
        boost::asio::io_context ioContext;
        boost::asio::ip::tcp::resolver resolver{ioContext};

        // ToDo: I hope its okay to share this across connections
        boost::asio::ssl::context sslContext{makeSslContext()};

    private:
        std::unique_ptr<SteamBot::HTTPClient::Response> performQuery(Params&, const RedirectedURL&);

    private:
        HTTPClient();
        ~HTTPClient();

        static HTTPClient& get();

    public:
        static boost::fibers::future<SteamBot::HTTPClient::ResponseType> query(SteamBot::HTTPClient::RequestType);
    };
}

/************************************************************************/

std::unique_ptr<SteamBot::HTTPClient::Response> HTTPClient::performQuery(HTTPClient::Params& params, const RedirectedURL& redirectedUrl)
{
    const boost::urls::url_view& url=redirectedUrl.url.empty() ? params.request->url : redirectedUrl.url;
    try
    {
        const std::string host=url.host();	// we need 0-termination for SSL_set_tlsext_host_name()

        std::string_view port=url.port();
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
            boost::beast::http::request<boost::beast::http::string_body> request{params.request->method, url.encoded_target(), 11};
            for (const auto& header : params.request->headers)
            {
                request.base().set(header.first, header.second);
            }
            if (!params.request->contentType.empty())
            {
                request.base().set("Content-Type", params.request->contentType);
            }
            if (!params.request->body.empty())
            {
                request.body()=std::move(params.request->body);
                request.content_length(request.body().size());
            }
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

        BOOST_LOG_TRIVIAL(info) << "HTTPClient: performQuery for \"" << url
                                << "\" has received a " << response->response.body().size()
                                << " byte response with code " << response->response.result();
        return response;
    }
    catch(const boost::system::system_error& exception)
    {
        BOOST_LOG_TRIVIAL(error) << "exception: " << exception.what();
        printSslError(exception.code());
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "exception: " << boost::current_exception_diagnostic_information();
    }

    BOOST_LOG_TRIVIAL(info) << "HTTPClient: performQuery for \"" << url << "\" has produced an error";
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

boost::fibers::future<SteamBot::HTTPClient::ResponseType> HTTPClient::query(SteamBot::HTTPClient::RequestType request)
{
    assert(request->url.scheme_id()==boost::urls::scheme::https);

    auto params=std::make_shared<Params>();
    params->request=std::move(request);

    HTTPClient::get().ioContext.post([params]() {
        try
        {
            RedirectedURL redirectedUrl;
            while (true)
            {
                auto result=HTTPClient::get().performQuery(*params, redirectedUrl);
                const auto code=result->response.result();
                if (code==boost::beast::http::status::moved_permanently || code==boost::beast::http::status::found)
                {
                    const auto& location=result->response.base()["Location"];
                    if (!location.empty())
                    {
                        if (code==boost::beast::http::status::moved_permanently)
                        {
                            BOOST_LOG_TRIVIAL(error) << "URL should be \"" << location << "\"";
                            assert(false);
                        }
                        redirectedUrl=RedirectedURL(location);
                        continue;
                    }
                }
                params->promise.set_value(std::move(result));
                return;
            }
        }
        catch(...)
        {
            params->promise.set_exception(std::current_exception());
        }
    });
    return params->promise.get_future();
}

/************************************************************************/

boost::fibers::future<SteamBot::HTTPClient::ResponseType> SteamBot::HTTPClient::query(SteamBot::HTTPClient::RequestType request)
{
    return ::HTTPClient::query(std::move(request));
}

/************************************************************************/
/*
 * Note: be careful with the url_view_base being non-owning!
 */

SteamBot::HTTPClient::Request::Request(boost::beast::http::verb method_, const boost::urls::url_view_base& url_)
    : method(method_), url(url_)
{
}

/************************************************************************/

SteamBot::HTTPClient::Request::~Request() =default;

/************************************************************************/

boost::json::value SteamBot::HTTPClient::parseJson(const SteamBot::HTTPClient::Response& response)
{
    boost::json::stream_parser parser;
    const auto buffers=response.response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        parser.write(static_cast<const char*>((*iterator).data()), (*iterator).size());
    }
    parser.finish();
    boost::json::value result=parser.release();
    BOOST_LOG_TRIVIAL(debug) << "response JSON body: " << result;
    return result;
}

/************************************************************************/

std::string SteamBot::HTTPClient::parseString(const SteamBot::HTTPClient::Response& response)
{
    std::string result;
    const auto buffers=response.response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        result.append(std::string_view{static_cast<const char*>((*iterator).data()), (*iterator).size()});
    }
    BOOST_LOG_TRIVIAL(debug) << "response string body has " << result.size() << " bytes";
    return result;
}
