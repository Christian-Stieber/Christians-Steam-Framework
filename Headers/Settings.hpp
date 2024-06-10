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

#pragma once

#include "Startup.hpp"
#include "Client/Client.hpp"

#include <string>
#include <string_view>
#include <map>

/************************************************************************/
/*
 * Every setting is described by Setting subclass.
 *
 * Each Setting class needs to provide a name, and functions to
 * convert to/from strings; this is used for storage and the UI. We
 * also provide some intermediate bases for specific types such as
 * "bool".
 *
 * Settings are stored in the whiteboard, as shared_ptr<const T>.
 *
 * Settings are stored in your account data, in the Settings/Name key.
 *
 * Create a static Init object to register your setting.
 */

/************************************************************************/

namespace SteamBot
{
    namespace Settings
    {
        class Setting
        {
        public:
            template <typename T=Setting> using Ptr=std::shared_ptr<const T>;
            typedef const SteamBot::Startup::InitBase<Setting> InitBase;

        public:
            const InitBase& init;

        public:
            Setting(const InitBase&);
            virtual ~Setting();

            virtual const std::string_view& name() const =0;

        public:
            // Internal use
            virtual bool setString(std::string_view) =0;
            virtual std::string_view getString() const =0;

            virtual void storeWhiteboard(Ptr<>) const =0;

        public:
            void load(const SteamBot::DataFile& dataFile);
        };
    }
}

/************************************************************************/
/*
 * Setting for an arbitrary string
 */

namespace SteamBot
{
    namespace Settings
    {
        class SettingString : public Setting
        {
        public:
            std::string value;

        public:
            using Setting::Setting;

        private:
            virtual bool setString(std::string_view) override;
            virtual std::string_view getString() const override;
        };
    }
}

/************************************************************************/
/*
 * This base class should help with creating boolean settings.
 */

namespace SteamBot
{
    namespace Settings
    {
        class SettingBool : public Setting
        {
        public:
            bool value=false;

        public:
            using Setting::Setting;

        private:
            virtual bool setString(std::string_view) override;
            virtual std::string_view getString() const override;
        };
    }
}

/************************************************************************/
/*
 * This base class should help with creating settings to target
 * another bot
 */

namespace SteamBot
{
    namespace Settings
    {
        class SettingBotName : public Setting
        {
        public:
            const SteamBot::ClientInfo* clientInfo=nullptr;

        public:
            using Setting::Setting;

        private:
            virtual bool setString(std::string_view) override;
            virtual std::string_view getString() const override;
        };
    }
}

/************************************************************************/
/*
 * Create a global instance of this to register the class.
 */

namespace SteamBot
{
    namespace Settings
    {
        template <typename T> using Init=SteamBot::Startup::Init<Setting, T>;
    }
}

/************************************************************************/
/*
 * This is mostly an interface to the UI.
 *
 * Note that this STILL needs to be called from the correct thread.
 */

namespace SteamBot
{
    namespace Settings
    {
        std::map<std::string_view, std::string> getValues();

        // pass an empty value to reset to default
        bool changeValue(std::string_view, std::string_view);
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Settings
    {
        namespace Internal
        {
            template <typename T> void storeWhiteboard(Setting::Ptr<> baseSetting)
            {
                if (Setting::Ptr<T> setting=std::dynamic_pointer_cast<const T>(std::move(baseSetting)))
                {
                    SteamBot::Client::getClient().whiteboard.set(std::move(setting));
                }
                else
                {
                    assert(false);
                }
            }
        }
    }
}
