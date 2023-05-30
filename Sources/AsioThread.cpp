#include "AsioThread.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::AsioThread AsioThread;

/************************************************************************/

AsioThread::AsioThread() =default;

/************************************************************************/

void AsioThread::launch()
{
    thread=std::thread([this]{
        BOOST_LOG_TRIVIAL(debug) << "AsioThread " << boost::typeindex::type_id_runtime(*this).pretty_name() << " launched";
        auto guard=boost::asio::make_work_guard(ioContext);
        ioContext.run();
        BOOST_LOG_TRIVIAL(debug) << "AsioThread " << boost::typeindex::type_id_runtime(*this).pretty_name() << " terminating";
    });
}

/************************************************************************/

AsioThread::~AsioThread()
{
    ioContext.stop();
    if (thread.joinable())
    {
        thread.join();
    }
}
