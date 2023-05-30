#pragma once

#include <thread>
#include <boost/asio/io_context.hpp>

/************************************************************************/

namespace SteamBot
{
    class AsioThread
    {
    private:
        std::thread thread;

    public:
        boost::asio::io_context ioContext;

    protected:
        void launch();

    public:
        AsioThread();
        virtual ~AsioThread();
    };
}
