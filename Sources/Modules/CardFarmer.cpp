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
#include "Modules/PackageData.hpp"
#include "UI/UI.hpp"
#include "Helpers/Time.hpp"

#include <unordered_set>

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
        class FarmInfo
        {
        public:
            const SteamBot::AppID appId=SteamBot::AppID::None;
            unsigned int cardsRemaining;

        public:
            FarmInfo(SteamBot::AppID appId_, unsigned int cardsRemaining_)
                : appId(appId_), cardsRemaining(cardsRemaining_)
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
#else
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
#endif
        };

    private:
        std::unordered_set<std::unique_ptr<FarmInfo>, FarmInfo::Hash, FarmInfo::Equal> games;

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

bool CardFarmerModule::FarmInfo::isRefundable() const
{
    if (const auto gameInfo=getInfo())
    {
        if (gameInfo->playtimeForever<=std::chrono::minutes(60+60+30))
        {
            const auto purchaseLimit=std::chrono::system_clock::now()-std::chrono::days(15);
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

void CardFarmerModule::FarmInfo::print(SteamBot::UI::OutputText& output) const
{
    output << SteamBot::toInteger(appId);
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

void CardFarmerModule::processBadgeData(const BadgeData& badgeData)
{
    decltype(games) myGames;

    unsigned int total=0;
    for (const auto& item : badgeData.badges)
    {
        if (auto cardsRemaining=item.second.cardsRemaining.value_or(0))
        {
            auto game=std::make_unique<FarmInfo>(item.first, cardsRemaining);

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
    SteamBot::UI::OutputText() << "you have " << total << " cards to farm across " << myGames.size() << " games";

    games=std::move(myGames);
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
