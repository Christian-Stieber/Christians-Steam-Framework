#pragma once

#include "Connection/Base.hpp"

#include <vector>

/************************************************************************/
/*
 * The TCP connection
 */

namespace SteamBot
{
    namespace Connection
    {
        class TCP : public Base
        {
        private:
            boost::asio::ip::tcp::socket socket;
            std::vector<MutableBytes::value_type> readBuffer;

        public:
            TCP();
            virtual ~TCP();

        public:
            virtual void connect(const Endpoint&) override;
            virtual void disconnect() override;
            virtual MutableBytes readPacket() override;
            virtual void writePacket(ConstBytes) override;

            virtual void getLocalAddress(Endpoint&) const override;

            virtual void cancel() override;
        };
    }
}
