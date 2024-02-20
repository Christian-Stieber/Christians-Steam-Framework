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
#include "Modules/GetBadgeData.hpp"
#include "Modules/OwnedGames.hpp"
#include "Modules/CardFarmer.hpp"
#include "UI/UI.hpp"

#include <set>

/************************************************************************/
/*
 * Max number of games to run in parallel
 */

static constexpr unsigned int maxGames=10;

/************************************************************************/

typedef SteamBot::Modules::GetPageData::Whiteboard::BadgeData BadgeData;
typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

/************************************************************************/

namespace
{
    class CardFarmerModule : public SteamBot::Client::Module
    {
    private:
        class GameInfo
        {
        private:
            SteamBot::AppID game=SteamBot::AppID::None;
            unsigned int cardsRemaining;
            std::chrono::minutes playTime;

        public:
            GameInfo(SteamBot::AppID game_, unsigned int cardsRemaining_, std::chrono::minutes playTime_)
                : game(game_), cardsRemaining(cardsRemaining_), playTime(playTime_)
            {
            }

            std::strong_ordering constexpr operator<=>(const GameInfo& other) const
            {
                if (cardsRemaining<other.cardsRemaining) return std::strong_ordering::less;
                if (cardsRemaining>other.cardsRemaining) return std::strong_ordering::greater;

                if (playTime>other.playTime) return std::strong_ordering::less;
                if (playTime<other.playTime) return std::strong_ordering::greater;

                return game<=>other.game;
            }

            bool constexpr operator==(const GameInfo& other) const { return operator<=>(other)==std::strong_ordering::equal; }
            bool constexpr operator<(const GameInfo& other) const { return operator<=>(other)==std::strong_ordering::less; }

        public:
            void output(const OwnedGames& ownedGames) const
            {
                auto gameInfo=ownedGames.getInfo(game);
                assert(gameInfo!=nullptr);

                SteamBot::UI::OutputText() << "\"" << gameInfo->name << "\" (" << SteamBot::toInteger(game) << ") has " << cardsRemaining << " cards left to get";
            }
        };

    private:
        std::vector<GameInfo> games;

    private:
        void processBadgeData(const BadgeData&);

    public:
        CardFarmerModule() =default;
        virtual ~CardFarmerModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    CardFarmerModule::Init<CardFarmerModule> init;
}

/************************************************************************/

void CardFarmerModule::processBadgeData(const BadgeData& badgeData)
{
    std::set<GameInfo> allGames;

    if (auto ownedGames=getClient().whiteboard.has<OwnedGames::Ptr>())
    {
        unsigned int total=0;
        for (const auto& item : badgeData.badges)
        {
            if (auto cardsRemaining=item.second.cardsRemaining.value_or(0))
            {
                if (auto ownedGame=(*ownedGames)->getInfo(item.first))
                {
                    auto result=allGames.emplace(item.first, cardsRemaining, ownedGame->playtimeForever).second;
                    assert(result);

                    total+=cardsRemaining;
                }
            }
        }

        SteamBot::UI::OutputText() << "you have " << total << " cards to farm across " << allGames.size() << " games";

        for (const auto& item : allGames)
        {
            item.output(**ownedGames);
        }
    }
}

/************************************************************************/

void CardFarmerModule::run(SteamBot::Client& client)
{
    BadgeData::Ptr currentBadgeData;

    auto badgeData=client.whiteboard.createWaiter<BadgeData::Ptr>(*waiter);

    while (true)
    {
        waiter->wait();

        if (auto newBadgeData=badgeData->has())
        {
            if (*newBadgeData!=currentBadgeData)
            {
                currentBadgeData=*newBadgeData;
                processBadgeData(*currentBadgeData);
            }
        }
    }
}

/************************************************************************/

void SteamBot::Modules::CardFarmer::use()
{
    SteamBot::Modules::GetPageData::use();
}
