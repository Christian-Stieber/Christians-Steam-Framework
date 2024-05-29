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

/************************************************************************/
/*
 * Every setting is described by Setting subclass.
 *
 * Each Setting class needs to provide a name, and functions to
 * convert to/from strings; this is used for storage and the UI. We
 * also provide some intermediate bases for specific types such as
 * "bool".
 *
 * Settings are stored in the whiteboard, as unique_ptr<const T>.
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
            template <typename T=Setting> using Ptr=std::unique_ptr<const T>;

        public:
            Setting();
            virtual ~Setting();

            virtual const std::string_view& name() const =0;

            virtual const std::string_view& getString() const =0;

        public:
            // Internal use
            virtual bool setString(std::string_view) =0;
            virtual void storeWhiteboard(Ptr<>) const =0;
        };
    }
}

/************************************************************************/
/*
 * Get whatever value is currently stored in the whiteboard.
 *
 * Since this returns a reference, whiteboard rules apply for the
 * lifetime.
 */

namespace SteamBot
{
    namespace Settings
    {
        template <typename T> const auto& getValue(SteamBot::Whiteboard::WaiterType<Setting::Ptr<T>>& waiter)
        {
            const auto& setting=waiter->get();
            return setting->value;
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Settings
    {
        class SettingBool : public Setting
        {
        public:
            bool value;

        public:
            SettingBool(bool value_=false);
            virtual ~SettingBool();

        private:
            virtual bool setString(std::string_view) override;
            virtual const std::string_view& getString() const override;
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Settings
    {
        template <typename T> using Init=SteamBot::Startup::Init<Setting, T>;
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
                if (auto setting=dynamic_cast<const T*>(baseSetting.get()))
                {
                    auto& whiteboard=SteamBot::Client::getClient().whiteboard;
                    baseSetting.release();
                    whiteboard.set<Setting::Ptr<T>>(setting);
                }
                else
                {
                    assert(false);
                }
            }
        }
    }
}
