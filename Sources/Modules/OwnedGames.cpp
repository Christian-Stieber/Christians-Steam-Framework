#include "Client/Module.hpp"
#include "Modules/OwnedGames.hpp"
#include "Modules/Login.hpp"
#include "Modules/UnifiedMessageClient.hpp"

#include "steamdatabase/protobufs/steam/steammessages_player.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

/************************************************************************/

namespace
{
    class OwnedGamesModule : public SteamBot::Client::Module
    {
    private:
        void getOwnedGames();

    public:
        OwnedGamesModule() =default;
        virtual ~OwnedGamesModule() =default;

        virtual void run() override;
    };

    OwnedGamesModule::Init<OwnedGamesModule> init;
}

/************************************************************************/

OwnedGames::OwnedGames() =default;
OwnedGames::~OwnedGames() =default;

OwnedGames::GameInfo::GameInfo() =default;
OwnedGames::GameInfo::~GameInfo() =default;

/************************************************************************/

boost::json::value OwnedGames::GameInfo::toJson() const
{
    boost::json::object json;
    SteamBot::enumToJson(json, "appId", appId);
    json["name"]=name;
    return json;
}

/************************************************************************/

boost::json::value OwnedGames::toJson() const
{
    boost::json::array json;
    for (const auto& item : games)
    {
        json.push_back(item.second->toJson());
    }
    return json;
}

/************************************************************************/

void OwnedGamesModule::getOwnedGames()
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Player::GetOwnedGames)> GetOwnedGamesInfo;
    std::shared_ptr<GetOwnedGamesInfo::ResultType> response;
    {
        GetOwnedGamesInfo::RequestType request;
        {
            auto sessionInfo=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SessionInfo>();
            if (sessionInfo)
            {
                request.set_steamid(sessionInfo->steamId->getValue());
            }
        }
        request.set_include_appinfo(true);
        request.set_include_played_free_games(true);
        response=SteamBot::Modules::UnifiedMessageClient::execute<GetOwnedGamesInfo::ResultType>("Player.GetOwnedGames#1", std::move(request));
    }

    OwnedGames ownedGames;

    for (int index=0; index<response->games_size(); index++)
    {
        const auto& gameData=response->games(index);
        if (gameData.has_appid())
        {
            auto game=std::make_shared<OwnedGames::GameInfo>();
            game->appId=static_cast<SteamBot::AppID>(gameData.appid());
            if (gameData.has_name())
            {
                game->name=gameData.name();
            }
            bool success=ownedGames.games.try_emplace(game->appId, std::move(game)).second;
            assert(success);
        }
    }

    getClient().whiteboard.set(std::move(ownedGames));
}

/************************************************************************/

void OwnedGamesModule::run()
{
    getClient().launchFiber("OwnedGamesModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;
        std::shared_ptr<SteamBot::Whiteboard::Waiter<LoginStatus>> loginStatus;
        loginStatus=waiter.createWaiter<decltype(loginStatus)::element_type>(getClient().whiteboard);

        while (true)
        {
            waiter.wait();
            if (loginStatus->get(LoginStatus::LoggedOut)==LoginStatus::LoggedIn)
            {
                getOwnedGames();
                BOOST_LOG_TRIVIAL(info) << "owned games: " << *(getClient().whiteboard.has<OwnedGames>());
            }
        }
    });
}
