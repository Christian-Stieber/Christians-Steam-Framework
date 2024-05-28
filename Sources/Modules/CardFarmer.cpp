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

#include <unordered_set>

/************************************************************************/
/*
 * Debugging: do we actually want to play games, or not
 */

#undef CHRISTIAN_PLAY_GAMES
// #define CHRISTIAN_PLAY_GAMES

/************************************************************************/
/*
 * Max number of games to run in parallel
 */

static constexpr unsigned int maxGames=10;

/************************************************************************/

// Max playtime for us to play games concurrently
static constinit std::chrono::minutes multipleGamesTime=std::chrono::hours(2);

// Refund limits, with some safety margin
static constinit std::chrono::minutes steamRefundPlayTime=std::chrono::minutes(2*60+30);
static constinit std::chrono::minutes steamRefundPurchaseTime=std::chrono::days(15);

/************************************************************************/

typedef SteamBot::Modules::BadgeData::Whiteboard::BadgeData BadgeData;
typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

/************************************************************************/

namespace
{
    class FarmInfo
    {
    public:
        const SteamBot::AppID appId=SteamBot::AppID::None;

        unsigned int cardsRemaining;
        unsigned int cardsReceived;

        std::chrono::steady_clock::time_point lastUpdate{std::chrono::steady_clock::now()};

    public:
        FarmInfo(SteamBot::AppID appId_, unsigned int cardsRemaining_, unsigned int cardsReceived_)
            : appId(appId_), cardsRemaining(cardsRemaining_), cardsReceived(cardsReceived_)
        {
        }

    public:
        std::shared_ptr<const OwnedGames::GameInfo> getInfo() const
        {
            return SteamBot::Modules::OwnedGames::getInfo(appId);
        }

    public:
        void print(SteamBot::UI::OutputText&) const;
        bool isRefundable() const;

    public:
#if 0
        class Less
        {
        public:
            bool operator()(const std::unique_ptr<FarmInfo>& left, const std::unique_ptr<FarmInfo>& right) const
            {
                return SteamBot::toInteger(left->appId)<SteamBot::toInteger(right->appid);
            }
        };
#endif

        class Hash
        {
        public:
            std::size_t operator()(const std::unique_ptr<FarmInfo>& key) const
            {
                return std::hash<decltype(key->appId)>{}(key->appId);
            }
        };

        class Equal
        {
        public:
            bool operator()(const std::unique_ptr<FarmInfo>& left, const std::unique_ptr<FarmInfo>& right) const
            {
                return SteamBot::toInteger(left->appId)==SteamBot::toInteger(right->appId);
            }
        };
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
    if (const auto gameInfo=getInfo())
    {
        if (gameInfo->playtimeForever<=steamRefundPlayTime)
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
    return true;
}

/************************************************************************/

void FarmInfo::print(SteamBot::UI::OutputText& output) const
{
    output << "CardFarmer: " << SteamBot::toInteger(appId);
    if (auto gameInfo=getInfo())
    {
        output << " (" << gameInfo->name << ") with "
               << SteamBot::Time::toString(gameInfo->playtimeForever) << " playtime has "
               << cardsRemaining << " cards left to get";
    }
    else
    {
        output << " has no GameInfo";
    }
}

/************************************************************************/

namespace
{
    class CardFarmerModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<BadgeData::Ptr> badgeDataWaiter;

    private:
        std::unordered_set<std::unique_ptr<FarmInfo>, FarmInfo::Hash, FarmInfo::Equal> games;	// all the games we want to farm
        std::vector<SteamBot::AppID> playing;

        // When playing multiple games, this when the first game should hit the 2 hour mark
        std::chrono::steady_clock::time_point twoHourMark;

    private:
        std::vector<SteamBot::AppID> selectMultipleGames(std::chrono::minutes&) const;
        FarmInfo* selectSingleGame(bool(*condition)(const FarmInfo&)) const;
        FarmInfo* selectSingleGame() const;

    private:
        void farmGames();
        void handleBadgeData();

    public:
        CardFarmerModule() =default;
        virtual ~CardFarmerModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    CardFarmerModule::Init<CardFarmerModule> init;
}

/************************************************************************/

void CardFarmerModule::handleBadgeData()
{
    if (auto badgeData=badgeDataWaiter->has())
    {
        decltype(games) myGames;

        unsigned int total=0;
        for (const auto& item : (*badgeData)->badges)
        {
            const auto cardsReceived=item.second.cardsReceived;
            const auto cardsEarned=item.second.cardsEarned;
            if (cardsReceived<cardsEarned)
            {
                const auto cardsRemaining=cardsEarned-cardsReceived;
                assert(cardsRemaining>0);

                auto game=std::make_unique<FarmInfo>(item.first, cardsRemaining, cardsReceived);

                SteamBot::UI::OutputText output;
                game->print(output);

                if (game->isRefundable())
                {
                    output << " (might still be refundable; ignoring)";
                }
                else
                {
                    auto result=myGames.insert(std::move(game)).second;
                    assert(result);

                    total+=cardsRemaining;
                }
            }
        }
        SteamBot::UI::OutputText() << "CardFarmer: you have " << total << " cards to farm across " << myGames.size() << " games";

        games=std::move(myGames);
    }
}

/************************************************************************/
/*
 * Find games for which condition returns true, and pick one with the
 * least remaining cards.
 */

FarmInfo* CardFarmerModule::selectSingleGame(bool(*condition)(const FarmInfo&)) const
{
    FarmInfo* candidate=nullptr;
    for (const auto& game : games)
    {
        if (game->cardsRemaining>0 && condition(*game))
        {
            if (candidate==nullptr || game->cardsRemaining<candidate->cardsRemaining)
            {
                candidate=game.get();
            }
        }
    }
    return candidate;
}

/************************************************************************/
/*
 *   If we have games that already have cards dropped:
 *     Select one with the least cards remaining
 *   Else, if we have games that already have more than 2 hours playtime:
 *     Select one with least cards remaining
 *   Else
 *     nullptr
 */

FarmInfo* CardFarmerModule::selectSingleGame() const
{
    FarmInfo* game=nullptr;

    if (game==nullptr)
    {
        game=selectSingleGame([](const FarmInfo& farmInfo) {
            return farmInfo.cardsReceived>0;
        });
    }

    if (game==nullptr)
    {
        game=selectSingleGame([](const FarmInfo& farmInfo) {
            if (auto gameInfo=farmInfo.getInfo())
            {
                if (gameInfo->playtimeForever>=multipleGamesTime)
                {
                    return true;
                }
            }
            return false;
        });
    }

    return game;
}

/************************************************************************/
/*
 * Select maxGames games with most playtime and least cards
 *
 * Returns the max playtime from these games.
 */

std::vector<SteamBot::AppID> CardFarmerModule::selectMultipleGames(std::chrono::minutes& playtime) const
{
    class Item
    {
    public:
        const FarmInfo* farmInfo;
        std::chrono::minutes playtime{0};

    public:
        Item(const FarmInfo* farmInfo_)
            : farmInfo(farmInfo_)
        {
            if (auto gameInfo=farmInfo->getInfo())
            {
                playtime=gameInfo->playtimeForever;
            }
            assert(farmInfo->cardsRemaining>0);
            assert(farmInfo->cardsReceived==0);
            assert(playtime<multipleGamesTime);
        }

    public:
        bool operator<(const Item& other) const
        {
            return std::tie(farmInfo->cardsRemaining, other.playtime)<std::tie(other.farmInfo->cardsRemaining, playtime);
        }
    };

    std::vector<Item> candidates;
    candidates.reserve(games.size());

    for (const auto& game : games)
    {
        candidates.emplace_back(game.get());
    }
    std::sort(candidates.begin(), candidates.end());

    std::vector<SteamBot::AppID> result;
    result.reserve(maxGames);
    playtime=std::chrono::minutes::zero();
    for (unsigned int i=0; i<maxGames && i<candidates.size(); i++)
    {
        result.push_back(candidates[i].farmInfo->appId);
        if (candidates[i].playtime>playtime)
        {
            playtime=candidates[i].playtime;
        }
    }
    return result;
}

/************************************************************************/
/*
 * Select up to maxGames games that we actually want to farm now,
 * and run them.
 *
 * Algorithm:
 *   If we have games that already have cards dropped:
 *     Select one with the least cards remaining
 *   Else, if we have games that already have more than 2 hours playtime:
 *     Select one with least cards remaining
 *   Else
 *     Select maxGames games with most playtime and least cards
 *
 * Also sets the twoHourMark, if necessary.
 */

void CardFarmerModule::farmGames()
{
    if (!games.empty())
    {
        std::vector<SteamBot::AppID> myGames;
        auto playDuration=std::chrono::minutes::max();

        twoHourMark=decltype(twoHourMark)();
        if (auto game=selectSingleGame())
        {
            myGames.push_back(game->appId);
        }
        else
        {
            std::chrono::minutes maxPlaytime;
            myGames=selectMultipleGames(maxPlaytime);
            assert(maxPlaytime<multipleGamesTime);

            if (myGames.size()>1)
            {
                playDuration=multipleGamesTime+std::chrono::minutes(1)-maxPlaytime;
                twoHourMark=decltype(twoHourMark)::clock::now()+playDuration;
            }
        }

        // Stop the games that we are no longer farming
        for (SteamBot::AppID appId : playing)
        {
            auto iterator=std::find(myGames.begin(), myGames.end(), appId);
            if (iterator==myGames.end())
            {
                SteamBot::Modules::PlayGames::Messageboard::PlayGame::play(appId, false);
            }
        }

        // And launch everything else. PlayGames will prevent duplicates.
        playing=std::move(myGames);

        {
            SteamBot::UI::OutputText output;
            output << "CardFarmer: playing";
            const char* separator=" ";
            for (SteamBot::AppID appId : playing)
            {
#ifdef CHRISTIAN_PLAY_GAMES
                xxx;
                SteamBot::Modules::PlayGames::Messageboard::PlayGame::play(appId, true);
#endif
                output << separator << appId;
                separator=", ";
            }
            if (playDuration!=decltype(playDuration)::max())
            {
                output << " for " << SteamBot::Time::toString(playDuration);
            }
        }
    }
}

/************************************************************************/

void CardFarmerModule::init(SteamBot::Client& client)
{
    badgeDataWaiter=client.whiteboard.createWaiter<BadgeData::Ptr>(*waiter);
}

/************************************************************************/

void CardFarmerModule::run(SteamBot::Client&)
{
    while (true)
    {
        if (twoHourMark==decltype(twoHourMark)())
        {
            waiter->wait();
        }
        else
        {
            waiter->wait<decltype(twoHourMark)::clock>(twoHourMark);
        }

        if (badgeDataWaiter->isWoken())
        {
            handleBadgeData();
        }

        farmGames();
    }
}

/************************************************************************/

void SteamBot::Modules::CardFarmer::use()
{
    SteamBot::Modules::BadgeData::use();
}
