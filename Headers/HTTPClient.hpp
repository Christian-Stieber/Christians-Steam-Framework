#pragma once

#include <memory>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/fiber/future/future.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace HTTPClient
    {
        // thrown for any error
        class Exception { };

        struct Response
        {
            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> response;
        };

        typedef std::unique_ptr<Response> ResponseType;
        boost::fibers::future<ResponseType> query(std::string_view);
    }
}
