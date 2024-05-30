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

Setting::Setting(const Setting::InitBase& init_)
    : init(init_)
{
}

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
        std::vector<std::shared_ptr<Setting>> settings;

    private:
        static SteamBot::DataFile& getDataFile();

        void loadItem(Setting&) const;
        void storeItem(const Setting&);
        void loadSettings();

    public:
        std::map<std::string_view, std::string> getValues() const;
        bool changeValue(std::string_view, std::string_view);

    public:
        SettingsModule();
        virtual ~SettingsModule() =default;
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

void SettingsModule::storeItem(const Setting& setting)
{
    dataFile.update([&setting](boost::json::value& json) -> bool {
        auto& value=SteamBot::JSON::createItem(json, settingsKey, setting.name());
        auto string=setting.getString();
        BOOST_LOG_TRIVIAL(info) << "Storing new value \"" << string << "\" into setting \"" << setting.name() << "\"";
        value.emplace_string()=std::move(setting.getString());
        return true;
    });
}

/************************************************************************/

void SettingsModule::loadItem(Setting& setting) const
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
    SteamBot::Startup::InitBase<Setting>::create([this](std::unique_ptr<Setting> created) {
        loadItem(*created);

        std::shared_ptr<Setting> setting{std::move(created)};
        setting->storeWhiteboard(setting);
        settings.push_back(std::move(setting));
    });
}

/************************************************************************/

SettingsModule::SettingsModule()
    : dataFile(getDataFile())
{
    loadSettings();
}

/************************************************************************/

std::map<std::string_view, std::string> SettingsModule::getValues() const
{
    std::map<std::string_view, std::string> result;
    for (const auto& setting: settings)
    {
        auto success=result.emplace(setting->name(), setting->getString()).second;
        assert(success);
    }
    return result;
}

/************************************************************************/

bool SettingsModule::changeValue(std::string_view name, std::string_view value)
{
    for (auto& setting: settings)
    {
        if (setting->name()==name)
        {
            if (setting->getString()==value)
            {
                // new value is the same as old
                return true;
            }

            auto newSetting=setting->init.createInstance();
            if (!value.empty())
            {
                if (!newSetting->setString(value))
                {
                    // invalid value
                    return false;
                }
            }

            storeItem(*newSetting);
            setting=std::move(newSetting);
            setting->storeWhiteboard(setting);
            return true;
        }
    }
    // invalid name
    return false;
}

/************************************************************************/

std::map<std::string_view, std::string> SteamBot::Settings::getValues()
{
    return SteamBot::Client::getClient().getModule<SettingsModule>()->getValues();
}

/************************************************************************/

bool SteamBot::Settings::changeValue(std::string_view name, std::string_view value)
{
    return SteamBot::Client::getClient().getModule<SettingsModule>()->changeValue(name, value);
}
