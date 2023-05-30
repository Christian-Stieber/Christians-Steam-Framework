#include "./Login.hpp"

#include "Config.hpp"
#include "ResultCode.hpp"
#include "Client/Module.hpp"
#include "Modules/Connection.hpp"
#include "Modules/Login.hpp"
#include "Modules/SteamGuard.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Modules::Connection::Whiteboard::ConnectionStatus ConnectionStatus;
typedef SteamBot::Modules::Connection::Whiteboard::LocalEndpoint LocalEndpoint;
typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;

/************************************************************************/

namespace
{
    class ResetException
    {
    };
}

/************************************************************************/

namespace
{
    class LoginModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Waiter waiter;
        struct Waiters
        {
            std::shared_ptr<SteamBot::Whiteboard::Waiter<ConnectionStatus>> connectionStatus;
            std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLogonResponseMessageType>> cmsgClientLogonResponse;
            std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientUpdateMachineAuthMessageType>> cmsgClientUpdateMachineAuth;

            void setup(SteamBot::Waiter&);
        } waiters;

    private:
        enum class Status {
            Start,
            SentClientLogon,
            WaitForRestart,
            LogonComplete
        } status=Status::Start;

    public:
        LoginModule() =default;
        virtual ~LoginModule() =default;

        virtual void run() override;

    private:
        void setStatus(LoginStatus status)
        {
            auto current=getClient().whiteboard.has<LoginStatus>();
            if (current==nullptr || *current!=status)
            {
                getClient().whiteboard.set(status);
            }
        }

        void progress();
        void setupWaiters();
        void sendClientLogon();
        void handleLoginResponse();
    };

    LoginModule::Init<LoginModule> init;
}

/************************************************************************/

void LoginModule::Waiters::setup(SteamBot::Waiter& waiter)
{
    auto& whiteboard=getClient().whiteboard;
    connectionStatus=waiter.createWaiter<decltype(connectionStatus)::element_type>(whiteboard);

    auto& messageboard=getClient().messageboard;
    cmsgClientLogonResponse=waiter.createWaiter<decltype(cmsgClientLogonResponse)::element_type>(messageboard);
    cmsgClientUpdateMachineAuth=waiter.createWaiter<decltype(cmsgClientUpdateMachineAuth)::element_type>(messageboard);
}

/************************************************************************/

void LoginModule::sendClientLogon()
{
    SteamBot::Modules::Login::Internal::LoginDetails details;
	{
		const auto& account=SteamBot::Config::SteamAccount::get();
		details.username=account.user;
		details.password=account.password;

        {
            auto steamGuardCode=getClient().whiteboard.has<SteamBot::Modules::SteamGuard::Whiteboard::SteamGuardCode>();
            assert(steamGuardCode!=nullptr);
            if (!steamGuardCode->empty())
            {
                details.steamGuardAuthCode=*steamGuardCode;
            }
        }

		details.sentryFileHash=SteamBot::Modules::Login::Internal::getSentryHash();
    }

    auto message=std::make_unique<Steam::CMsgClientLogonMessageType>();
	{
        auto const localEndpoint=getClient().whiteboard.has<LocalEndpoint>();
        if (localEndpoint==nullptr)
        {
            throw ResetException();
        }
        SteamBot::Modules::Login::Internal::fillClientLogonMessage(*message, details, *localEndpoint);
	}

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
    status=Status::SentClientLogon;
}

/************************************************************************/

template <typename T> static void ignoreMessages(std::shared_ptr<SteamBot::Messageboard::Waiter<T>>& waiter)
{
    while (true)
    {
        auto message=waiter->fetch();
        if (message==nullptr)
        {
            break;
        }
        BOOST_LOG_TRIVIAL(info) << "ignoring unexpected message: " << boost::typeindex::type_id<T>().pretty_name();
    }
}

/************************************************************************/

void LoginModule::handleLoginResponse()
{
    auto message=waiters.cmsgClientLogonResponse->fetch();
    if (message)
    {
        if (message->content.has_eresult())
        {
            auto loginResult=static_cast<SteamBot::ResultCode>(static_cast<std::underlying_type_t<SteamBot::ResultCode>>(message->content.eresult()));
            BOOST_LOG_TRIVIAL(debug) << "login result code: " << SteamBot::enumToStringAlways(loginResult);
            switch(loginResult)
            {
            case SteamBot::ResultCode::AccountLogonDenied:
            case SteamBot::ResultCode::InvalidLoginAuthCode:
                status=Status::WaitForRestart;
                SteamBot::Modules::SteamGuard::registerAccount();
                break;

            case SteamBot::ResultCode::OK:
                // SteamKit CMClient.cs -> HandleLogOnResponse
                {
                    SteamBot::Modules::Login::Whiteboard::SessionInfo sessionInfo;
                    if (message->header.proto.has_steamid()) sessionInfo.steamId=SteamBot::SteamID(message->header.proto.steamid());
                    if (message->header.proto.has_client_sessionid()) sessionInfo.sessionId=message->header.proto.client_sessionid();
                    if (message->content.has_cell_id()) sessionInfo.cellId=message->content.cell_id();
                    getClient().whiteboard.set(std::move(sessionInfo));
                }
#if 0
                PublicIP = logonResp.Body.public_ip.GetIPAddress();
                IPCountryCode = logonResp.Body.ip_country_code;
#endif
                if (message->content.has_legacy_out_of_game_heartbeat_seconds())
                {
                    const auto seconds=message->content.legacy_out_of_game_heartbeat_seconds();
                    assert(seconds>0);
                    typedef SteamBot::Modules::Login::Whiteboard::HeartbeatInterval HeartbeatInterval;
                    getClient().whiteboard.set<HeartbeatInterval>(std::chrono::seconds(seconds));
                }

                setStatus(LoginStatus::LoggedIn);
                status=Status::LogonComplete;
                break;

            default:
                assert(false);
                break;
            }
        }
    }
}

/************************************************************************/

void LoginModule::progress()
{
    if (waiters.connectionStatus->get(ConnectionStatus::Disconnected)==ConnectionStatus::Connected)
    {
        while (true)
        {
            auto cmsgClientUpdateMachineAuth=waiters.cmsgClientUpdateMachineAuth->fetch();
            if (!cmsgClientUpdateMachineAuth)
            {
                break;
            }
            SteamBot::Modules::Login::Internal::handleCmsgClientUpdateMachineAuth(std::move(cmsgClientUpdateMachineAuth));
        }

        switch(status)
        {
        case Status::Start:
            sendClientLogon();
            ignoreMessages(waiters.cmsgClientLogonResponse);
            break;

        case Status::SentClientLogon:
            handleLoginResponse();
            break;

        case Status::WaitForRestart:
            break;

        case Status::LogonComplete:
            break;
        }
    }
    else
    {
        setStatus(LoginStatus::LoggedOut);
    }
}

/************************************************************************/

void LoginModule::run()
{
    setStatus(LoginStatus::LoggedOut);
    getClient().launchFiber("LoginModule::run", [this](){
        waiters.setup(waiter);
        auto cancellation=getClient().cancel.registerObject(waiter);
        while (true)
        {
            try
            {
                progress();
                waiter.wait();
            }
            catch(const ResetException&)
            {
            }
        }
    });
}
