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

#include <string_view>
#include <optional>
#include <unordered_map>

#include <boost/fiber/mutex.hpp>

/************************************************************************/
/*
 * Client-settings are stored with the client data; global settings
 * are not currently implemented.
 *
 * The idea is that modules "use" a setting, which makes it available
 * for other interested parties. If a setting was not "used", query
 * functions will report it as unset, while set functions will be
 * ignored (even if the setting actually exists in the client data).
 */

/************************************************************************/
/*
 * ToDo: we probably need a notification when settings change...
 */

/************************************************************************/
/*
 * IMPORTANT: when calling "use", you MUST use a static string
 */

/************************************************************************/

namespace SteamBot
{
    class DataFile;

    class SettingsBase
    {
    public:
        enum Type { Invalid, Bool, Integer, String };

    protected:
        SettingsBase();
        virtual ~SettingsBase();

    protected:
        mutable boost::fibers::mutex mutex;
        std::unordered_map<std::string_view, Type> settings;

    public:
        bool checkSetting(std::string_view, Type) const;
        Type getType(std::string_view) const;

    public:
        void use(std::string_view, Type);

    protected:
        void clear(SteamBot::DataFile&, std::string_view) const;

    protected:
        std::optional<bool> getBool(SteamBot::DataFile&, std::string_view) const;
        void setBool(SteamBot::DataFile&, std::string_view, bool) const;

    public:
        typedef std::pair<decltype(settings)::key_type, decltype(settings)::mapped_type> ListPair;
        std::vector<ListPair> getVariables() const;
    };
}

/************************************************************************/

namespace SteamBot
{
    class ClientSettings : public SettingsBase
    {
    public:
        // This is sent as a message on the client
        class Changed
        {
        public:
            std::string name;

        public:
            static void send(SteamBot::DataFile&, std::string_view);
        };

    private:
        ClientSettings();

    public:
        virtual ~ClientSettings();

    public:
        static ClientSettings& get();

    public:
        void clear(std::string_view, std::string_view) const;
        void clear(std::string_view) const;

    public:
        std::optional<bool> getBool(std::string_view, std::string_view) const;
        std::optional<bool> getBool(std::string_view) const;

        void setBool(std::string_view, std::string_view, bool) const;
        void setBool(std::string_view, bool) const;
    };
}
