#include "Client/Module.hpp"
#include "Modules/SteamGuard.hpp"
#include "UI/UI.hpp"
#include "Config.hpp"

#include <mutex>
#include <unordered_set>

/************************************************************************/

namespace
{
    std::mutex mutex;
    std::unordered_set<std::string> needsSteamGuard;		// account names
}

/************************************************************************/

namespace
{
    class SteamGuardModule : public SteamBot::Client::Module
    {
    private:
        bool isFlagged();

    public:
        SteamGuardModule() =default;
        virtual ~SteamGuardModule() =default;

        virtual void run() override;
    };

    SteamGuardModule::Init<SteamGuardModule> init;
}

/************************************************************************/
/*
 * Check whether the current account needs SteamGuard, and also reset
 * the registration.
 */

bool SteamGuardModule::isFlagged()
{
    const auto& account=SteamBot::Config::SteamAccount::get().user;
    std::lock_guard<decltype(mutex)> lock(mutex);
    return (needsSteamGuard.erase(account)!=0);
}

/************************************************************************/

void SteamGuardModule::run()
{
    getClient().launchFiber("SteamGuardModule::run", [this](){
        std::string code;
        if (isFlagged())
        {
            SteamBot::Waiter waiter;
            auto cancellation=getClient().cancel.registerObject(waiter);

            auto steamGuardCode=SteamBot::UI::GetSteamguardCode::create(waiter);

            waiter.wait();
            code=steamGuardCode->fetch();
        }
        typedef SteamBot::Modules::SteamGuard::Whiteboard::SteamGuardCode SteamGuardCode;
        getClient().whiteboard.set<SteamGuardCode>(std::move(code));
    });
}

/************************************************************************/

void SteamBot::Modules::SteamGuard::registerAccount()
{
    const auto& account=SteamBot::Config::SteamAccount::get().user;
    BOOST_LOG_TRIVIAL(info) << "SteamGuard: registering account " << account;
    std::lock_guard<decltype(mutex)> lock(mutex);
    needsSteamGuard.insert(account);
}
