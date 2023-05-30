#include "Client/Module.hpp"
#include "Client/Messageboard.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_Messageboard : public SteamBot::Client::Module
    {
    public:
        class TickMessage { };

    public:
    TestModule_Messageboard() =default;
        virtual ~TestModule_Messageboard() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                for (int i=0; i<5; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::seconds(1));
                    BOOST_LOG_TRIVIAL(debug) << "messageboard: sending TickMessage";
                    getClient().messageboard.send(std::make_shared<TickMessage>());
                }
            });
            getClient().launchFiber([this](){
                SteamBot::Waiter waiter;
                auto ticker=waiter.createWaiter<SteamBot::Messageboard::Waiter<TickMessage>>(getClient().messageboard);
                while (true)
                {
                    waiter.wait();
                    if (ticker->fetch())
                    {
                        BOOST_LOG_TRIVIAL(debug) << "messageboard: received TickMessage";
                    }
                }
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_Messageboard::Init<TestModule_Messageboard> init;
}
