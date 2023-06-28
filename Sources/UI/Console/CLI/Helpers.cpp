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

#include "./Helpers.hpp"

#include "Modules/Executor.hpp"
#include "Modules/PackageData.hpp"

#include <charconv>

/************************************************************************/

typedef CLI::Helpers Helpers;

/************************************************************************/

Helpers::Helpers(CLI& cli_)
    : cli(cli_)
{
}

Helpers::~Helpers() =default;

/************************************************************************/

std::vector<std::shared_ptr<const Helpers::LicenseInfo>> Helpers::getLicenseInfo(const SteamBot::ClientInfo& clientInfo, SteamBot::AppID appId)
{
    std::vector<std::shared_ptr<const LicenseInfo>> result;
    if (auto client=clientInfo.getClient())
    {
        SteamBot::Modules::Executor::execute(std::move(client), [appId, &result](SteamBot::Client& client) mutable {
            auto packages=SteamBot::Modules::PackageData::getPackageInfo(appId);
            for (const auto& package : packages)
            {
                if (auto licenseInfo=SteamBot::Modules::LicenseList::getLicenseInfo(package->packageId))
                {
                    result.emplace_back(std::move(licenseInfo));
                }
            }
        });
    }
    return result;
}

/************************************************************************/

Helpers::OwnedGames::Ptr Helpers::getOwnedGames(const SteamBot::ClientInfo& clientInfo)
{
    OwnedGames::Ptr ownedGames;
    if (auto client=clientInfo.getClient())
    {
        SteamBot::Modules::Executor::execute(std::move(client), [&ownedGames](SteamBot::Client& client) mutable {
            if (auto games=client.whiteboard.has<decltype(ownedGames)>())
            {
                ownedGames=*games;
            }
        });
    }
    return ownedGames;
}

/************************************************************************/

bool Helpers::parseNumber(std::string_view string, uint64_t& value)
{
    const auto last=string.data()+string.size();
    const auto result=std::from_chars(string.data(), last, value);
    if (result.ec==std::errc() && result.ptr==last)
    {
        return true;
    }
    std::cout << "\"" << string << "\" is not a valid number" << std::endl;
    return false;
}
