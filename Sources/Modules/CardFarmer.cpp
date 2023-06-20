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
#include "UI/UI.hpp"

/************************************************************************/

typedef SteamBot::Modules::GetPageData::Whiteboard::BadgeData BadgeData;

/************************************************************************/

namespace
{
    class CardFarmerModule : public SteamBot::Client::Module
    {
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
    typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
    auto ownedGames=getClient().whiteboard.get<OwnedGames::Ptr>(nullptr);

    unsigned int total=0;
    for (const auto& item : badgeData.badges)
    {
        auto remaining=item.second.cardsRemaining.value_or(0);
        if (remaining>0)
        {
            SteamBot::UI::OutputText output;

            output << static_cast<std::underlying_type_t<decltype(item.first)>>(item.first);

            if (ownedGames)
            {
                if (auto info=ownedGames->getInfo(item.first))
                {
                    output << " (" << info->name << ")";
                }
            }

            output << " has " << remaining << " cards left";
            total+=remaining;
        }
    }

    if (total>0)
    {
        SteamBot::UI::OutputText() << "you have a total of " << total << " cards to farm";
    }
    else
    {
        SteamBot::UI::OutputText() << "you have no cards to farm";
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
