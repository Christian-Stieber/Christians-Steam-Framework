#include "Client/Module.hpp"
#include "HTTPClient.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class TestModule_HTTPClient : public SteamBot::Client::Module
    {
    public:
        TestModule_HTTPClient() =default;
        virtual ~TestModule_HTTPClient() =default;

        virtual void run() override
        {
            getClient().launchFiber([this](){
                // ToDo: why does this not fail ssl verification?
                auto response=SteamBot::HTTPClient::query("https://gatekeeper:2443/public/Test").get();
            });
        }
    };
}

/************************************************************************/

namespace
{
    TestModule_HTTPClient::Init<TestModule_HTTPClient> init;
}
