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
 * In all selection steps below, if we find more suitable games than
 * we need, we prefer low remaining cards and high playtime.
 *
 * Among the games that are farmable we first try to select a single
 * game to farm:
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
#define CHRISTIAN_PLAY_GAMES

/************************************************************************/

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

    public:
        std::shared_ptr<const OwnedGames::GameInfo> getInfo() const
        {
            return SteamBot::Modules::OwnedGames::getInfo(appId);
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

#if 0
void FarmInfo::print(SteamBot::UI::OutputText& output) const
{
    output << "CardFarmer: " << appId << ": "
           << SteamBot::Time::toString(playtime) << " playtime, "
           << cardsReceived << " cards received, "
           << cardsRemaining << " remaining";
}
#endif

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
                    game->playtime=info->playtimeForever;
                    game->cardsRemaining=cardsRemaining;
                    game->cardsReceived=cardsReceived;

                    bool refundable=game->isRefundable();

#if 0
                    {
                        SteamBot::UI::OutputText output;
                        game->print(output);
                        if (refundable)
                        {
                            output << " (might still be refundable; ignoring)";
                        }
                    }
#endif

                    if (!refundable)
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
        if (game->cardsRemaining>0 && condition(*game))
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
        if (game->cardsRemaining>0 && game->playtime>=multipleGamesTimeMin && game->playtime<multipleGamesTimeMax)
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
 * Select which games we want to farm
 */

std::vector<SteamBot::AppID> CardFarmerModule::selectFarmGames() const
{
    std::vector<SteamBot::AppID> myGames;

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
                stop.push_back(appId);
            }
        }
        SteamBot::Modules::PlayGames::Messageboard::PlayGames::play(std::move(stop), false);
    }

    // And launch everything else. PlayGames will prevent duplicates.
    std::sort(myGames.begin(), myGames.end());
    if (myGames!=playing)
    {
        playing=std::move(myGames);

#if 0
        {
            SteamBot::UI::OutputText output;
            output << "CardFarmer: ";
            if (playing.empty())
            {
                output << "nothing to farm";
            }
            else
            {
                output << "playing ";
                const char* separator="";
                for (SteamBot::AppID appId : playing)
                {
                    output << separator << appId;
                    separator=", ";
                }
            }
        }
#endif

#ifdef CHRISTIAN_PLAY_GAMES
        SteamBot::Modules::PlayGames::Messageboard::PlayGames::play(playing, true);
#endif
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

void CardFarmerModule::handle(std::shared_ptr<const GameChanged> message)
{
    if (!message->newGame)
    {
        if (auto game=findGame(message->appId))
        {
            if (auto info=game->getInfo())
            {
                game->playtime=info->playtimeForever;
            }
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
        waiter->wait();

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
