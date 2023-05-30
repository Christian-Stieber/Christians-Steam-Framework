#include "Client/Module.hpp"
#include "Client/Whiteboard.hpp"

#include <boost/log/trivial.hpp>

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
                for (int i=0; i<5; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::seconds(1));
                    getClient().whiteboard.set<int>(100+i);
                }
            });
            getClient().launchFiber([this](){
                for (unsigned i=0; i<7; i++)
                {
                    boost::this_fiber::sleep_for(std::chrono::milliseconds(700));
                    getClient().whiteboard.set<unsigned>(200+i);
                }
            });
            getClient().launchFiber([this](){
                SteamBot::Waiter waiter;
                auto Int=waiter.createWaiter<SteamBot::Whiteboard::Waiter<int>>(getClient().whiteboard);
                auto Unsigned=waiter.createWaiter<SteamBot::Whiteboard::Waiter<unsigned>>(getClient().whiteboard);
                while (true)
                {
                    BOOST_LOG_TRIVIAL(debug) << "whiteboard: " << Int->get(0) << ", " << Unsigned->get(0);
                    waiter.wait();
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
