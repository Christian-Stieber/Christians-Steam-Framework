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

#include <boost/beast/version.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

/************************************************************************/
/*
 * This is the basic HTTPClient handling to execute a single query;
 * it's expected to run on the Asio thread already, and does not
 * perform any rate limiting.
 */

/************************************************************************/

typedef SteamBot::HTTPClient::Internal::BasicQuery BasicQuery;

namespace http=boost::beast::http;

/************************************************************************/

static inline unsigned long getSslErrorCode(const boost::system::error_code& error)
{
    auto errorValue=error.value();
    assert(errorValue>=0);
    return static_cast<unsigned long>(errorValue);
}

/************************************************************************/

boost::asio::ssl::context& BasicQuery::getSslContext()
{
    static boost::asio::ssl::context& sslContext=*([](){
        auto context=new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client);
#ifndef _WIN32
        context->set_default_verify_paths();
        context->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
#else
        // ToDo: Windows-openssl doesn't seem to have certificates?
        context->set_verify_mode(boost::asio::ssl::verify_none);
#endif
        return context;
    }());

    return sslContext;
}

/************************************************************************/

BasicQuery::BasicQuery(HTTPClient::Query& query_, Callback&& callback_)
    : query(&query_),
      callback(std::move(callback_)),
      resolver(SteamBot::Asio::getIoContext())
{
    assert(SteamBot::Asio::isThread());
    BOOST_LOG_TRIVIAL(debug) << "constructed query to " << query->url;
}

/************************************************************************/

BasicQuery::~BasicQuery()
{
    BOOST_LOG_TRIVIAL(debug) << "destroying query";
}

/************************************************************************/
/*
 * https://stackoverflow.com/questions/9828066/how-to-decipher-a-boost-asio-ssl-error-code
 */

#include <openssl/err.h>
#include <boost/lexical_cast.hpp>

static void printSslError(const boost::system::error_code& error)
{
    if (error.category() == boost::asio::error::get_ssl_category()) {
        std::string err = error.message();
        const auto errorCode=getSslErrorCode(error);
        err = std::string(" (")
            +boost::lexical_cast<std::string>(ERR_GET_LIB(errorCode))+","
            //+boost::lexical_cast<std::string>(ERR_GET_FUNC(errorCode))+","
            +boost::lexical_cast<std::string>(ERR_GET_REASON(errorCode))+") "
            ;
        //ERR_PACK /* crypto/err/err.h */
        char buf[128];
        ::ERR_error_string_n(getSslErrorCode(error), buf, sizeof(buf));
        err += buf;
        BOOST_LOG_TRIVIAL(error) << "ssl error: " << err;
    }
}

/************************************************************************/

void BasicQuery::complete(const ErrorCode& error)
{
    assert(SteamBot::Asio::isThread());

    if (error)
    {
        BOOST_LOG_TRIVIAL(error) << "query has failed with " << error.message() << " (" << error << ")";
        printSslError(error);
    }

    query->error=error;
    callback(*this);
}

/************************************************************************/

void BasicQuery::close()
{
    // Gracefully close the stream
    // Note: shutdown can hang: https://github.com/boostorg/beast/issues/995

    auto timer=std::make_shared<boost::asio::steady_timer>(SteamBot::Asio::getIoContext(), std::chrono::seconds(2));
    auto self=shared_from_this();

    timer->async_wait([self](const ErrorCode&) {
        self->stream->next_layer().cancel();
    });

    stream->async_shutdown([self=std::move(self), timer=std::move(timer)](const ErrorCode& error) {
        BOOST_LOG_TRIVIAL(debug) << "ssl shutdown completed with " << error;
    });
}

/************************************************************************/

void BasicQuery::read_completed(const ErrorCode& error, size_t)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(info) << "HTTPClient: \"" << query->request.method_string()
                            << "\" query for \"" << query->url
                            << "\" has received a " << query->response.body().size()
                            << " byte response with code \"" << query->response.result() << "\"";

    close();

    {
        std::ostringstream output;
        const char* separator="received headers:";
        for (const auto& field : query->response)
        {
            output << separator << " [" << field.name_string() << ": " << field.value() << "]";
            separator=",";
        }
        BOOST_LOG_TRIVIAL(debug) << output.view();
    }

    if (query->cookies)
    {
        query->cookies->update(query->url, query->response);
        BOOST_LOG_TRIVIAL(debug) << "new cookie jar: " << query->cookies->toJson();
    }

    complete(ErrorCode());
}

/************************************************************************/

void BasicQuery::write_completed(const ErrorCode& error, size_t bytes)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "write_completed with " << bytes << " bytes";

    // Receive the HTTP response
    query->responseBuffer=decltype(query->responseBuffer)();
    query->response=decltype(query->response)();
    boost::beast::http::async_read(*stream, query->responseBuffer, query->response, std::bind_front(&BasicQuery::read_completed, shared_from_this()));
}

/************************************************************************/

void BasicQuery::handshake_completed(const ErrorCode& error)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "handshake_completed";

    query->request.target(query->url.encoded_target());
    query->request.set(http::field::host, host);
    query->request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    if (query->request[http::field::accept_language]=="")
    {
        query->request.set(http::field::accept_language, "en-US; q=0.9, en; q=0.8");
    }

    if (query->cookies)
    {
        std::string jarCookies=query->cookies->get(query->url);
        if (!jarCookies.empty())
        {
            std::string cookies;
            cookies=query->request.operator[](http::field::cookie);

            if (!cookies.empty()) cookies.push_back(';');
            cookies.append(std::move(jarCookies));

            query->request.set(http::field::cookie, cookies);
        }
    }

    {
        std::ostringstream output;
        const char* separator="outgoing headers:";
        for (const auto& field : query->request)
        {
            output << separator << " [" << field.name_string() << ": " << field.value() << "]";
            separator=",";
        }
        BOOST_LOG_TRIVIAL(debug) << output.view();
    }

    // Send the HTTP request to the remote host
    http::async_write(*stream, query->request, std::bind_front(&BasicQuery::write_completed, shared_from_this()));
}

/************************************************************************/

void BasicQuery::connect_completed(const ErrorCode& error, const Resolver::endpoint_type& endpoint)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "connect_completed with endpoint " << endpoint;

    // Perform the SSL handshake
    // ToDo: for some reason, certificate problems don't seem to matter...
    stream->async_handshake(boost::asio::ssl::stream_base::client, std::bind_front(&BasicQuery::handshake_completed, shared_from_this()));
}

/************************************************************************/

void BasicQuery::resolve_completed(const ErrorCode& error, Resolver::results_type resolverResults)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "resolve_completed";

    // Setup the SSL stream
    stream=std::make_unique<decltype(stream)::element_type>(SteamBot::Asio::getIoContext(), getSslContext());
    if (!SSL_set_tlsext_host_name(stream->native_handle(), host.c_str()))
    {
        const boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        return complete(error);
    }

    // Make the connection on an IP address we got from the lookup
    boost::beast::get_lowest_layer(*stream).async_connect(resolverResults, std::bind_front(&BasicQuery::connect_completed, shared_from_this()));
}

/************************************************************************/

void BasicQuery::perform()
{
    std::string_view port=query->url.port();
    if (port.empty()) port="443";

    // Look up the host name
    host=query->url.host();
    resolver.async_resolve(host, port, std::bind_front(&BasicQuery::resolve_completed, shared_from_this()));
}
