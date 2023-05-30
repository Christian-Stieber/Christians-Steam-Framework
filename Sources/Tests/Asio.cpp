#include "Client/Module.hpp"
#include "AsioYield.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

/************************************************************************/

namespace
{
    class TestModule : public SteamBot::Client::Module
    {
    public:
        TestModule() =default;
        virtual ~TestModule() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                boost::asio::deadline_timer timer(getClient().getIoContext());
                while (true)
                {
                    timer.expires_from_now(boost::posix_time::seconds(1));
                    timer.async_wait(boost::fibers::asio::yield);
                    BOOST_LOG_TRIVIAL(debug) << "asio timer fired";
                }
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule::Init<TestModule> init;
}
