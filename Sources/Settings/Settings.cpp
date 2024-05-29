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
#include "Helpers/JSON.hpp"
#include "Settings.hpp"

/************************************************************************/

typedef SteamBot::Settings::Setting Setting;

/************************************************************************/

static const std::string_view settingsKey="Settings";

/************************************************************************/

Setting::Setting() =default;
Setting::~Setting() =default;

/************************************************************************/
/*
 * For now(?), the module only loads the settings.
 */

namespace
{
    class SettingsModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::DataFile& dataFile;

    private:
        static SteamBot::DataFile& getDataFile();

        void loadItem(Setting&);
        void loadSettings();

    public:
        SettingsModule();
        virtual ~SettingsModule() =default;

    private:
        SteamBot::DataFile& getFile() const;
    };

    SettingsModule::Init<SettingsModule> init;
}

/************************************************************************/

SteamBot::DataFile& SettingsModule::getDataFile()
{
    std::string_view accountName=SteamBot::Client::getClient().getClientInfo().accountName;
    return SteamBot::DataFile::get(accountName, SteamBot::DataFile::FileType::Account);
}

/************************************************************************/

void SettingsModule::loadItem(Setting& setting)
{
    dataFile.examine([&setting](const boost::json::value& json) {
        if (auto item=SteamBot::JSON::getItem(json, settingsKey, setting.name()))
        {
            if (auto string=item->if_string())
            {
                if (setting.setString(*string))
                {
                    BOOST_LOG_TRIVIAL(info) << "Setting \"" << setting.name() << "\": "
                                            << "loaded value \"" << setting.getString() << "\"";
                }
                else
                {
                    BOOST_LOG_TRIVIAL(error) << "Setting \"" << setting.name() << "\": "
                                             << "ignored invalid value \"" << *string << "\"";
                }
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "Setting \"" << setting.name() << "\": "
                                         << "ignored non-string json value";
            }
        }
    });
}

/************************************************************************/

void SettingsModule::loadSettings()
{
    SteamBot::Startup::InitBase<Setting>::create([this](std::unique_ptr<Setting> settingPtr) {
        auto& setting=*settingPtr;
        loadItem(setting);
        setting.storeWhiteboard(std::move(settingPtr));
    });
}

/************************************************************************/

SettingsModule::SettingsModule()
    : dataFile(getDataFile())
{
    loadSettings();
}
