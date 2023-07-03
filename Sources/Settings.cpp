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

#include "Settings.hpp"
#include "DataFile.hpp"
#include "Client/Client.hpp"
#include "Helpers/JSON.hpp"

/************************************************************************/

const std::string_view settingsKey="Settings";

/************************************************************************/

SteamBot::SettingsBase::SettingsBase() =default;
SteamBot::SettingsBase::~SettingsBase() =default;

/************************************************************************/

void SteamBot::SettingsBase::use(std::string_view name, SteamBot::SettingsBase::Type type)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto result=settings.emplace(name, type);
    assert(result.second);	// duplicate registers are probably bad
}

/************************************************************************/

SteamBot::SettingsBase::Type SteamBot::SettingsBase::getType(std::string_view settingName) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    auto iterator=settings.find(settingName);
    if (iterator!=settings.end())
    {
        return iterator->second;
    }
    return Type::Invalid;
}

/************************************************************************/

bool SteamBot::SettingsBase::checkSetting(std::string_view settingName, SteamBot::SettingsBase::Type type) const
{
    auto myType=getType(settingName);
    if (myType!=Type::Invalid)
    {
        if (myType==type)
        {
            return true;
        }
        assert(false);	// type mismatch is bad
    }
    return false;
}

/************************************************************************/

void SteamBot::SettingsBase::clear(SteamBot::DataFile& file, std::string_view settingName) const
{
    file.update([&settingName](boost::json::value& json) {
        SteamBot::JSON::eraseItem(json, settingsKey, settingName);
        return true;
    });
}

/************************************************************************/

std::optional<bool> SteamBot::SettingsBase::getBool(SteamBot::DataFile& file, std::string_view settingName) const
{
    std::optional<bool> result;
    if (checkSetting(settingName, Type::Bool))
    {
        file.examine([&result, &settingName](const boost::json::value& json) mutable {
            if (auto item=SteamBot::JSON::getItem(json, settingsKey, settingName))
            {
                if (auto boolItem=item->if_bool())
                {
                    result.emplace(*boolItem);
                }
            }
        });
    }
    return result;
}

/************************************************************************/

void SteamBot::SettingsBase::setBool(SteamBot::DataFile& file, std::string_view settingName, bool value) const
{
    if (checkSetting(settingName, Type::Bool))
    {
        file.update([&settingName, value](boost::json::value& json) {
            SteamBot::JSON::createItem(json, settingsKey, settingName)=value;
            return true;
        });
    }
}

/************************************************************************/

std::vector<SteamBot::SettingsBase::ListPair> SteamBot::SettingsBase::getVariables() const
{
    std::vector<ListPair> result;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        result.reserve(settings.size());
        result.insert(result.end(), settings.begin(), settings.end());
    }
    return result;
}

/************************************************************************/

SteamBot::ClientSettings::ClientSettings() =default;
SteamBot::ClientSettings::~ClientSettings() =default;

/************************************************************************/

static std::string_view getAccountName()
{
    return SteamBot::Client::getClient().getClientInfo().accountName;
}

/************************************************************************/

SteamBot::ClientSettings& SteamBot::ClientSettings::get()
{
    static auto& instance=*new ClientSettings;
    return instance;
}

/************************************************************************/

void SteamBot::ClientSettings::clear(std::string_view settingName) const
{
    clear(settingName, getAccountName());
}

/************************************************************************/

std::optional<bool> SteamBot::ClientSettings::getBool(std::string_view settingName) const
{
    return getBool(settingName, getAccountName());
}

/************************************************************************/

void SteamBot::ClientSettings::setBool(std::string_view settingName, bool value) const
{
    setBool(settingName, getAccountName(), value);
}

/************************************************************************/

void SteamBot::ClientSettings::clear(std::string_view settingName, std::string_view accountName) const
{
    auto& file=SteamBot::DataFile::get(accountName, SteamBot::DataFile::FileType::Account);
    SettingsBase::clear(file, settingName);
}

/************************************************************************/

std::optional<bool> SteamBot::ClientSettings::getBool(std::string_view settingName, std::string_view accountName) const
{
    auto& file=SteamBot::DataFile::get(accountName, SteamBot::DataFile::FileType::Account);
    return SettingsBase::getBool(file, settingName);
}

/************************************************************************/

void SteamBot::ClientSettings::setBool(std::string_view settingName, std::string_view accountName, bool value) const
{
    auto& file=SteamBot::DataFile::get(accountName, SteamBot::DataFile::FileType::Account);
    SettingsBase::setBool(file, settingName, value);
}
