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

#include "Modules/SaleSticker.hpp"
#include "Modules/Executor.hpp"

/************************************************************************/

namespace
{
    class SaleStickerCommand : public CLI::CLICommandBase
    {
    public:
        SaleStickerCommand(CLI& cli_)
            : CLICommandBase(cli_, "sale-sticker", "", "claim a sale sticker", true)
        {
        }

        virtual ~SaleStickerCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    SaleStickerCommand::InitClass<SaleStickerCommand> init;
}

/************************************************************************/

bool SaleStickerCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        if (auto client=clientInfo->getClient())
        {
            bool success=SteamBot::Modules::Executor::executeWithFiber(client, [](SteamBot::Client& client) {
                auto json=SteamBot::SaleSticker::claim().toJson();
                SteamBot::UI::OutputText() << "Sale sticker: " << json;
            });
            if (success)
            {
                std::cout << "requested sale sticker for account " << client->getClientInfo().accountName << std::endl;
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

void SteamBot::UI::CLI::useSaleStickerCommand()
{
}
