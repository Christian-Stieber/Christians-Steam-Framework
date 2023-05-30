#include "Client/Module.hpp"
#include "WebAPI/ISteamDirectory/GetCMList.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_GetCMList : public SteamBot::Client::Module
    {
    public:
        TestModule_GetCMList() =default;
        virtual ~TestModule_GetCMList() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                auto cmlist=SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0);
                BOOST_LOG_TRIVIAL(debug) << "got GetCMList";
                boost::this_fiber::sleep_for(std::chrono::milliseconds(5000));
                cmlist=SteamBot::WebAPI::ISteamDirectory::GetCMList::get(0);
                BOOST_LOG_TRIVIAL(debug) << "got GetCMList";
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_GetCMList::Init<TestModule_GetCMList> init;
}
