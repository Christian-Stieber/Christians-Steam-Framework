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

#include "Asio/Asio.hpp"
#include "Asio/HTTPClient.hpp"
#include "Client/Client.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

#include <boost/log/trivial.hpp>
#include <boost/json/stream_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef boost::system::error_code ErrorCode;
typedef boost::asio::ip::tcp::resolver Resolver;

namespace HTTPClient=SteamBot::HTTPClient;
namespace http=boost::beast::http;

/************************************************************************/

namespace
{
    class Query : public std::enable_shared_from_this<Query>
    {
    public:
        typedef std::function<void()> Callback;

    private:
        HTTPClient::Query* query;
        Callback callback;

    private:
        // we need 0-termination for SSL_set_tlsext_host_name()
        std::string host;

    private:
        boost::asio::io_context& ioContext;
        Resolver resolver;
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream;

    private:
        static boost::asio::ssl::context& getSslContext();

    public:
        Query(HTTPClient::Query& query_, Callback&& callback_)
            : query(&query_),
              callback(std::move(callback_)),
              host(query->url.host()),
              ioContext(SteamBot::Asio::getIoContext()),
              resolver(ioContext),
              stream(ioContext, getSslContext())
        {
            assert(SteamBot::Asio::isThread());
            BOOST_LOG_TRIVIAL(debug) << "constructed query to " << query->url;
        }

        ~Query()
        {
            assert(query==nullptr);
            BOOST_LOG_TRIVIAL(debug) << "destroying query";
        }

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

/************************************************************************/

boost::asio::ssl::context& Query::getSslContext()
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

static inline unsigned long getSslErrorCode(const boost::system::error_code& error)
{
    auto errorValue=error.value();
    assert(errorValue>=0);
    return static_cast<unsigned long>(errorValue);
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

void Query::complete(const ErrorCode& error)
{
    assert(SteamBot::Asio::isThread());

    if (error)
    {
        BOOST_LOG_TRIVIAL(error) << "query has failed with " << error.message() << " (" << error << ")";
        printSslError(error);
    }

    assert(query!=nullptr);
    query->error=error;
    query=nullptr;
    callback();
}

/************************************************************************/

void Query::close()
{
    // Gracefully close the stream
    // Note: shutdown can hang: https://github.com/boostorg/beast/issues/995

    auto timer=std::make_shared<boost::asio::steady_timer>(ioContext, std::chrono::seconds(2));
    auto self=shared_from_this();

    timer->async_wait([self](const ErrorCode&) {
        self->stream.next_layer().cancel();
    });

    stream.async_shutdown([self=std::move(self), timer=std::move(timer)](const ErrorCode& error) {
        BOOST_LOG_TRIVIAL(debug) << "ssl shutdown completed with " << error;
    });
}

/************************************************************************/

void Query::read_completed(const ErrorCode& error, size_t)
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

void Query::write_completed(const ErrorCode& error, size_t bytes)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "write_completed with " << bytes << " bytes";

    // Receive the HTTP response
    query->responseBuffer=decltype(query->responseBuffer)();
    query->response=decltype(query->response)();
    boost::beast::http::async_read(stream, query->responseBuffer, query->response, std::bind_front(&Query::read_completed, shared_from_this()));
}

/************************************************************************/

void Query::handshake_completed(const ErrorCode& error)
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
    http::async_write(stream, query->request, std::bind_front(&Query::write_completed, shared_from_this()));
}

/************************************************************************/

void Query::connect_completed(const ErrorCode& error, const Resolver::endpoint_type& endpoint)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "connect_completed with endpoint " << endpoint;

    // Perform the SSL handshake
    // ToDo: for some reason, certificate problems don't seem to matter...
    stream.async_handshake(boost::asio::ssl::stream_base::client, std::bind_front(&Query::handshake_completed, shared_from_this()));
}

/************************************************************************/

void Query::resolve_completed(const ErrorCode& error, Resolver::results_type resolverResults)
{
    if (error)
    {
        return complete(error);
    }

    BOOST_LOG_TRIVIAL(debug) << "resolve_completed";

    // Setup the SSL stream
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
    {
        const boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        return complete(error);
    }

    // Make the connection on an IP address we got from the lookup
    boost::beast::get_lowest_layer(stream).async_connect(resolverResults, std::bind_front(&Query::connect_completed, shared_from_this()));
}

/************************************************************************/

void Query::perform()
{
    std::string_view port=query->url.port();
    if (port.empty()) port="443";

    // Look up the host name
    resolver.async_resolve(host, port, std::bind_front(&Query::resolve_completed, shared_from_this()));
}

/************************************************************************/

HTTPClient::Query::Query(boost::beast::http::verb method, boost::urls::url url_)
    : url(std::move(url_)), request(method, "", 11)
{
}

/************************************************************************/

HTTPClient::Query::~Query() =default;

/************************************************************************/

boost::json::value SteamBot::HTTPClient::parseJson(const SteamBot::HTTPClient::Query& query)
{
    assert(query.response.result()==boost::beast::http::status::ok);

    boost::json::stream_parser parser;
    const auto buffers=query.response.body().cdata();
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

std::string SteamBot::HTTPClient::parseString(const SteamBot::HTTPClient::Query& query)
{
    assert(query.response.result()==boost::beast::http::status::ok);

    std::string result;
    const auto buffers=query.response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        result.append(std::string_view{static_cast<const char*>((*iterator).data()), (*iterator).size()});
    }
    BOOST_LOG_TRIVIAL(debug) << "response string body has " << result.size() << " bytes";
    return result;
}

/************************************************************************/

HTTPClient::Query::QueryPtr HTTPClient::perform(HTTPClient::Query::QueryPtr query)
{
    auto waiter=SteamBot::Waiter::create();
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(*waiter);

    auto responseWaiter=SteamBot::HTTPClient::perform(waiter, std::move(query));
    while (true)
    {
        waiter->wait();
        if (auto response=responseWaiter->getResult())
        {
            return std::move(*response);
        }
    }
}

/************************************************************************/

static bool checkRetry(HTTPClient::Query& query)
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
 * For some reason, I want to serialize the http queries, and also
 * slow them down a bit.
 */

namespace
{
    class Queue
    {
    private:
        typedef std::shared_ptr<HTTPClient::Query::WaiterType> ItemType;

        std::chrono::steady_clock::time_point lastQuery;
        std::queue<ItemType> queue;

        boost::asio::steady_timer timer{SteamBot::Asio::getIoContext()};

    public:
        void performNext()
        {
            assert(SteamBot::Asio::isThread());
            if (!queue.empty())
            {
                auto now=decltype(lastQuery)::clock::now();
                auto next=lastQuery+std::chrono::seconds(5);
                if (now<next)
                {
                    auto cancelled=timer.expires_at(next);
                    assert(cancelled==0);
                    timer.async_wait([this](const boost::system::error_code& error) {
                        assert(!error);
                        performNext();
                    });
                }
                else
                {
                    auto& item=queue.front();
                    auto query=std::make_shared<::Query>(*(item->setResult()),[this, item]() {
                        lastQuery=decltype(lastQuery)::clock::now();
                        assert(!queue.empty() && queue.front()==item);
                        if (!checkRetry(*(item->setResult())))
                        {
                            item->completed();
                            queue.pop();
                        }
                        performNext();
                    });
                    query->perform();
                }
            }
        }

    public:
        void enqueue(ItemType item)
        {
            SteamBot::Asio::post("HTTPClient/Queue::enqueue", [this, item]() {
                queue.push(std::move(item));
                if (queue.size()==1)
                {
                    performNext();
                }
            });
        }
    };
}

/************************************************************************/

std::shared_ptr<HTTPClient::Query::WaiterType> HTTPClient::perform(std::shared_ptr<SteamBot::WaiterBase> waiter, HTTPClient::Query::QueryPtr query)
{
    static Queue& queue=*new Queue;

    auto result=waiter->createWaiter<HTTPClient::Query::WaiterType>();
    result->setResult()=std::move(query);
    queue.enqueue(result);
    return result;
}
