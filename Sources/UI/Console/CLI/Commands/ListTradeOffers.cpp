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

#include "UI/CLI.hpp"
#include "../Helpers.hpp"

#include "Modules/TradeOffers.hpp"
#include "Modules/Executor.hpp"
#include "AssetData.hpp"
#include "EnumString.hpp"

/************************************************************************/

namespace
{
    class ListTradeOffersCommand : public CLI::CLICommandBase
    {
    public:
        ListTradeOffersCommand(CLI& cli_)
            : CLICommandBase(cli_, "list-tradeoffers", "", "show incoming trade offers", true)
        {
        }

        virtual ~ListTradeOffersCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    ListTradeOffersCommand::InitClass<ListTradeOffersCommand> init;
}

/************************************************************************/

static void printOffers(const SteamBot::TradeOffers::TradeOffers& offers)
{
    struct PrintItems
    {
        static void print(SteamBot::UI::OutputText& output, const std::vector<std::shared_ptr<SteamBot::TradeOffers::TradeOffer::Item>>& items)
        {
            for (const auto& item : items)
            {
                output << "         ";
                if (item->amount>1)
                {
                    output << item->amount << "× ";
                }
                if (auto info=SteamBot::AssetData::query(item))
                {
                    output << "\"" << info->type << "\" / \"" << info->name << "\" (" << SteamBot::enumToString(info->itemType) << ")";
                }
                else
                {
                    output << "(unidentified item)";
                }
                output << "\n";
            }
        }
    };

    const char* direction=nullptr;
    const char* partnerLabel=nullptr;

    switch(offers.direction)
    {
    case SteamBot::TradeOffers::TradeOffers::Direction::Incoming:
        direction="incoming";
        partnerLabel="from";
        break;

    case SteamBot::TradeOffers::TradeOffers::Direction::Outgoing:
        direction="outgoing";
        partnerLabel="to";
        break;
    }

    if (offers.offers.size()>0)
    {
        SteamBot::UI::OutputText output;
        output << offers.offers.size() << " " << direction << " trade offers:\n";
        for (const auto& offer : offers.offers)
        {
            output << "   id " << toInteger(offer.second->tradeOfferId);
            output << " " << partnerLabel << " " << SteamBot::ClientInfo::prettyName(offer.second->partner) << ":\n";
            output << "      my items:\n";
            PrintItems::print(output, offer.second->myItems);
            output << "      for their items:\n";
            PrintItems::print(output, offer.second->theirItems);
        }
    }
    else
    {
        SteamBot::UI::OutputText() << "no " << direction << " trade offers";
    }
}

/************************************************************************/

bool ListTradeOffersCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        if (auto client=clientInfo->getClient())
        {
            bool success=SteamBot::Modules::Executor::executeWithFiber(client, [](SteamBot::Client& client) {
                if (auto offers=SteamBot::TradeOffers::getIncoming())
                {
                    printOffers(*offers);
                }
                else
                {
                    SteamBot::UI::OutputText() << "no incoming trade offers";
                }
                if (auto offers=SteamBot::TradeOffers::getOutgoing())
                {
                    printOffers(*offers);
                }
                else
                {
                    SteamBot::UI::OutputText() << "no outgoing trade offers";
                }
            });
            if (success)
            {
                std::cout << "requested trade offers for account " << client->getClientInfo().accountName << std::endl;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useListTradeOffersCommand()
{
}
