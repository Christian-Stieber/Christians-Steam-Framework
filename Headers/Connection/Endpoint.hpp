#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <string_view>

/************************************************************************/

namespace SteamBot
{
    namespace Connection
    {
        class Endpoint
        {
        public:
            boost::asio::ip::address address;
            uint16_t port=0;

        public:
            Endpoint();
            Endpoint(const boost::asio::ip::tcp::endpoint&);
            Endpoint(const boost::asio::ip::udp::endpoint&);

            ~Endpoint();

        public:
            class InvalidStringException { };
            Endpoint(std::string_view);
            Endpoint(const std::string&);

        public:
            operator boost::asio::ip::tcp::endpoint() const;
            operator boost::asio::ip::udp::endpoint() const;
        };
    }
}
