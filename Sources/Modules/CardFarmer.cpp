/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "Client/Module.hpp"
#include "Modules/BadgeData.hpp"
#include "Modules/OwnedGames.hpp"
#include "Modules/CardFarmer.hpp"
#include "Modules/PackageData.hpp"
#include "Modules/PlayGames.hpp"
#include "UI/UI.hpp"
#include "Helpers/Time.hpp"

#include <unordered_map>

/************************************************************************/
/*
 * Our farming strategy is as follows.
 *
 * As a main rule, we only run a game for at most "farmIntervalTime"
 * (currently 10 minutes). We then stop it, and give it a pause of
 * up to "pauseTime", or until Steam gives us a playtime update.
 *
 * Also, in all selection steps below, if we find more suitable games
 * than we need, we prefer low remaining cards and high playtime.
 *
 * Among the games that are farmable (i.e. not paused), we first try
 * to select a single game to farm:
 *   1) find a game that already has received cards
 *   2) find a game with more than "multipleGamesTimeMax" playtime
 *   3) find a game with less than "multipleGamesTimeMin" playtime
 *
 * If we did not find a game, we select up to "maxGames" games that
 * currently have a playtime between "multipleGamesTimeMin" and
 * "multipleGamesTimeMax".
 */

/************************************************************************/
/*
 * Debugging: do we actually want to play games, or not
 */

#undef CHRISTIAN_PLAY_GAMES
// #define CHRISTIAN_PLAY_GAMES

/************************************************************************/
/*
 * Some clock-related types
 */

typedef std::chrono::steady_clock Clock;
typedef Clock::time_point TimePoint;
typedef std::chrono::minutes Duration;

/************************************************************************/
/*
 * Max number of games to run in parallel
 */

static constexpr unsigned int maxGames=2;

/************************************************************************/
/*
 * This defines a playtime range where we just add playtime by
 * running multiple games.
 */

static constinit Duration multipleGamesTimeMin=std::chrono::minutes(30);
static constinit Duration multipleGamesTimeMax=std::chrono::hours(2);

/************************************************************************/

#ifdef CHRISTIAN_PLAY_GAMES
static constinit Duration farmIntervalTime=std::chrono::minutes(10);
#else
static constinit Duration farmIntervalTime=std::chrono::minutes(2);
#endif

static constinit Duration pauseTime=std::chrono::minutes(1);

/************************************************************************/
/*
 * Refund limits, with some safety margin
 */

static constinit Duration steamRefundPlayTime=std::chrono::minutes(2*60+30);
static constinit Duration steamRefundPurchaseTime=std::chrono::days(15);

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData BadgeData;
typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
typedef SteamBot::Modules::OwnedGames::Messageboard::GameChanged GameChanged;

/************************************************************************/

typedef SteamBot::Modules::CardFarmer::Settings::Enable Enable;
SteamBot::Settings::Init<Enable> init_enable;

/************************************************************************/

void Enable::storeWhiteboard(Enable::Ptr<> setting) const
{
    SteamBot::Settings::Internal::storeWhiteboard<Enable>(std::move(setting));
}

/************************************************************************/

const std::string_view& Enable::name() const
{
    static const std::string_view string="card-farmer-enable";
    return string;
}

/************************************************************************/

namespace
{
    class FarmInfo
    {
    public:
        const SteamBot::AppID appId=SteamBot::AppID::None;

        unsigned int cardsRemaining=0;
        unsigned int cardsReceived=0;
        Duration playtime{0};

        bool isPaused=false;
        TimePoint nextStatusCheck{TimePoint::max()};

    public:
        std::shared_ptr<const OwnedGames::GameInfo> getInfo() const
        {
            return SteamBot::Modules::OwnedGames::getInfo(appId);
        }

        void gotUpdate();

    public:
        bool isFarmable() const
        {
            return cardsRemaining>0 && !isPaused;
        }

    public:
        void clearFarmTiming()
        {
            isPaused=false;
            nextStatusCheck=TimePoint::max();
        }

    public:
        void print(SteamBot::UI::OutputText&) const;
        bool isRefundable() const;

    public:
        bool operator<(const FarmInfo& other) const
        {
            return std::tie(cardsRemaining, other.playtime) < std::tie(other.cardsRemaining, playtime);
        }
    };
}

/************************************************************************/
/*
 * I'm not entirely sure how to determine refundability, so for now
 * I'm considering a game to be refundable if it has a playtime of
 * less than 2.5 hours, and there is at least one license that's not
 * paid by "ActivationKey" or "Complimentary" and has a purchase date
 * less than 15 days ago (approximate; I don't use calendar data).
 *
 * Note: this is a somewhat expensive operation, so we just ignore
 * these games right away to avoid having to re-check.
 *
 * Also, in case of missing data, we assume it's refundable.
 */

bool FarmInfo::isRefundable() const
{
    if (playtime<=steamRefundPlayTime)
    {
        const auto purchaseLimit=std::chrono::system_clock::now()-steamRefundPurchaseTime;
        const auto packages=SteamBot::Modules::PackageData::getPackageInfo(appId);
        for (const auto& package : packages)
        {
            if (auto licenseInfo=SteamBot::Modules::LicenseList::getLicenseInfo(package->packageId))
            {
                if (licenseInfo->paymentMethod!=SteamBot::PaymentMethod::ActivationCode &&
                    licenseInfo->paymentMethod!=SteamBot::PaymentMethod::Complimentary)
                {
                    if (licenseInfo->timeCreated>=purchaseLimit)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/************************************************************************/

void FarmInfo::print(SteamBot::UI::OutputText& output) const
{
    output << "CardFarmer: " << appId << ": "
           << SteamBot::Time::toString(playtime) << " playtime, "
           << cardsReceived << " cards received, "
           << cardsRemaining << " remaining";
}

/************************************************************************/

namespace
{
    class CardFarmerModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<BadgeData::Ptr> badgeDataWaiter;
        SteamBot::Whiteboard::WaiterType<Enable::Ptr<Enable>> enableWaiter;
        SteamBot::Messageboard::WaiterType<GameChanged> gameChangedWaiter;

    private:
        std::unordered_map<SteamBot::AppID, std::unique_ptr<FarmInfo>> games;	// all the games we want to farm
        std::vector<SteamBot::AppID> playing;

    private:
        FarmInfo* selectSingleGame(bool(*)(const FarmInfo&)) const;
        std::vector<SteamBot::AppID> selectMultipleGames() const;
        std::vector<SteamBot::AppID> selectFarmGames() const;
        TimePoint findNextStatusCheck() const;
        void handleFarmTimings() const;
        void playGames(std::vector<SteamBot::AppID>);
        void farmGames();
        void handleBadgeData();

        FarmInfo* findGame(SteamBot::AppID) const;

    public:
        void handle(std::shared_ptr<const GameChanged>);

    public:
        CardFarmerModule() =default;
        virtual ~CardFarmerModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    CardFarmerModule::Init<CardFarmerModule> init_module;
}

/************************************************************************/

FarmInfo* CardFarmerModule::findGame(SteamBot::AppID appId) const
{
    auto iterator=games.find(appId);
    if (iterator!=games.end())
    {
        return iterator->second.get();
    }
    return nullptr;
}

/************************************************************************/

void CardFarmerModule::handleBadgeData()
{
    decltype(games) myGames;
    if (auto badgeData=badgeDataWaiter->has())
    {
        unsigned int total=0;
        for (const auto& item : (*badgeData)->badges)
        {
            const auto cardsReceived=item.second.cardsReceived;
            const auto cardsEarned=item.second.cardsEarned;
            if (cardsReceived<cardsEarned)
            {
                const auto cardsRemaining=cardsEarned-cardsReceived;
                assert(cardsRemaining>0);

                std::unique_ptr<FarmInfo> game;
                {
                    auto iterator=games.find(item.first);
                    if (iterator!=games.end())
                    {
                        game=std::move(iterator->second);
                    }
                }
                if (!game)
                {
                    game=std::make_unique<FarmInfo>(item.first);
                }
                if (auto info=game->getInfo())
                {
                    if (game->playtime!=info->playtimeForever ||
                        game->cardsRemaining!=cardsRemaining ||
                        game->cardsReceived!=cardsReceived)
                    {
                        game->playtime=info->playtimeForever;
                        game->cardsRemaining=cardsRemaining;
                        game->cardsReceived=cardsReceived;
                        game->clearFarmTiming();
                    }

                    SteamBot::UI::OutputText output;
                    game->print(output);

                    if (game->isRefundable())
                    {
                        output << " (might still be refundable; ignoring)";
                    }
                    else
                    {
                        auto result=myGames.emplace(item.first, std::move(game)).second;
                        assert(result);

                        total+=cardsRemaining;
                    }
                }
            }
        }

        SteamBot::UI::OutputText() << "CardFarmer: you have " << total << " cards to farm across " << myGames.size() << " games";
    }
    games=std::move(myGames);
}

/************************************************************************/
/*
 * Select a game that matches condition
 */

FarmInfo* CardFarmerModule::selectSingleGame(bool(*condition)(const FarmInfo&)) const
{
    FarmInfo* candidate=nullptr;
    for (const auto& item : games)
    {
        FarmInfo* game=item.second.get();
        if (game->isFarmable() && condition(*game))
        {
            if (candidate==nullptr || *game<*candidate)
            {
                candidate=game;
            }
        }
    }
    return candidate;
}

/************************************************************************/
/*
 * Pick up to maxNumber games with playtimes between
 * multipleGamesTimeMin and multipleGamesTimeMax.
 */

std::vector<SteamBot::AppID> CardFarmerModule::selectMultipleGames() const
{
    std::vector<FarmInfo*> candidates;

    for (const auto& item : games)
    {
        FarmInfo* game=item.second.get();
        if (game->isFarmable() && game->playtime>=multipleGamesTimeMin && game->playtime<multipleGamesTimeMax)
        {
            candidates.push_back(game);
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const FarmInfo* left, const FarmInfo* right) {
        return *left<*right;
    });

    std::vector<SteamBot::AppID> result;
    result.reserve(maxGames);
    for (FarmInfo* game: candidates)
    {
        result.push_back(game->appId);
        if (result.size()>=maxGames)
        {
            break;
        }
    }
    return result;
}

/************************************************************************/
/*
 * This will unpause the games where the timeout has been reached,
 * and pause games that have reached their farmIntervalTime.
 */

void CardFarmerModule::handleFarmTimings() const
{
    auto now=Clock::now();
    for (const auto& item: games)
    {
        auto& game=*(item.second);
        if (game.nextStatusCheck<=now)
        {
            if (game.isPaused)
            {
                game.clearFarmTiming();
            }
            else
            {
                game.isPaused=true;
                game.nextStatusCheck=now+pauseTime;
            }
        }
    }
}

/************************************************************************/
/*
 * Select which games we want to farm
 */

std::vector<SteamBot::AppID> CardFarmerModule::selectFarmGames() const
{
    std::vector<SteamBot::AppID> myGames;

    handleFarmTimings();

    if (auto game=selectSingleGame([](const FarmInfo& candidate) { return candidate.cardsReceived>0; }))
    {
        myGames.push_back(game->appId);
    }
    else if ((game=selectSingleGame([](const FarmInfo& candidate) { return candidate.playtime>=multipleGamesTimeMax; })))
    {
        myGames.push_back(game->appId);
    }
    else if ((game=selectSingleGame([](const FarmInfo& candidate) { return candidate.playtime<multipleGamesTimeMin; })))
    {
        myGames.push_back(game->appId);
    }
    else
    {
        myGames=selectMultipleGames();
    }

    return myGames;
}

/************************************************************************/
/*
 * Among our games, find the next time to perform a status check.
 * Status cehcks are:
 *   - a pause timeout
 *   - a game having been farmed for farmInterval minutes
 */

TimePoint CardFarmerModule::findNextStatusCheck() const
{
    TimePoint result{TimePoint::max()};
    for (const auto& item: games)
    {
        const FarmInfo& game=*(item.second);
#if 0
        if (game.nextStatusCheck!=TimePoint::max())
        {
            SteamBot::UI::OutputText() << game.appId << " wakeup in " << std::chrono::duration_cast<std::chrono::seconds>(game.nextStatusCheck-Clock::now()).count();
        }
#endif
        if (game.nextStatusCheck<result)
        {
            result=game.nextStatusCheck;
        }
    }
    return result;
}

/************************************************************************/
/*
 * Update our playing games, so we are playing the listed games
 */

void CardFarmerModule::playGames(std::vector<SteamBot::AppID> myGames)
{
    // Stop the games that we are no longer playing
    {
        std::vector<SteamBot::AppID> stop;
        for (SteamBot::AppID appId : playing)
        {
            auto iterator=std::find(myGames.begin(), myGames.end(), appId);
            if (iterator==myGames.end())
            {
                if (FarmInfo* game=findGame(appId))
                {
                    if (!game->isPaused)
                    {
                        game->clearFarmTiming();
                    }
                }
                stop.push_back(appId);
            }
        }
        SteamBot::Modules::PlayGames::Messageboard::PlayGames::play(std::move(stop), false);
    }

    // And launch everything else. PlayGames will prevent duplicates.
    playing=std::move(myGames);

    if (!playing.empty())
    {
        {
            SteamBot::UI::OutputText output;
            output << "CardFarmer: playing ";
            const char* separator="";
            for (SteamBot::AppID appId : playing)
            {
                output << separator << appId;
                separator=", ";
            }
        }

#ifdef CHRISTIAN_PLAY_GAMES
        xxx;
        SteamBot::Modules::PlayGames::Messageboard::PlayGames::play(playing, true);
#endif

        auto now=Clock::now();
        for (SteamBot::AppID appId : playing)
        {
            if (auto game=findGame(appId))
            {
                assert(!game->isPaused);
                if (game->nextStatusCheck==TimePoint::max())
                {
                    game->nextStatusCheck=now+farmIntervalTime;
                }
            }
        }
    }
}

/************************************************************************/
/*
 * Select games to farm, and play them
 */

void CardFarmerModule::farmGames()
{
    std::vector<SteamBot::AppID> myGames;
    if (getClient().whiteboard.get<Enable::Ptr<Enable>>()->value)
    {
        myGames=selectFarmGames();
    }
    playGames(std::move(myGames));
}

/************************************************************************/

void FarmInfo::gotUpdate()
{
    if (auto info=getInfo())
    {
        if (playtime!=info->playtimeForever)
        {
            playtime=info->playtimeForever;
            clearFarmTiming();
        }
    }
}

/************************************************************************/

void CardFarmerModule::handle(std::shared_ptr<const GameChanged> message)
{
    if (!message->newGame)
    {
        if (auto game=findGame(message->appId))
        {
            game->gotUpdate();
        }
    }
}

/************************************************************************/

void CardFarmerModule::init(SteamBot::Client& client)
{
    badgeDataWaiter=client.whiteboard.createWaiter<BadgeData::Ptr>(*waiter);
    enableWaiter=client.whiteboard.createWaiter<Enable::Ptr<Enable>>(*waiter);
    gameChangedWaiter=client.messageboard.createWaiter<GameChanged>(*waiter);
}

/************************************************************************/

void CardFarmerModule::run(SteamBot::Client&)
{
    waitForLogin();

    while (true)
    {
        waiter->wait<Clock>(findNextStatusCheck());

        if (badgeDataWaiter->isWoken())
        {
            handleBadgeData();
        }

        gameChangedWaiter->handle(this);

        enableWaiter->has();

        farmGames();
    }
}

/************************************************************************/

void SteamBot::Modules::CardFarmer::use()
{
    SteamBot::Modules::BadgeData::use();
}
