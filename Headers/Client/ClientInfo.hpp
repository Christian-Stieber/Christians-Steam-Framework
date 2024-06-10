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

#include "Client/Client.hpp"
#include "Settings.hpp"

/************************************************************************/

namespace SteamBot
{
    class AccountDisplayName : public SteamBot::Settings::SettingString
    {
    public:
        typedef Setting::Ptr<AccountDisplayName> Ptr;

    public:
        using SettingString::SettingString;

    private:
        virtual const std::string_view& name() const override;
        virtual void storeWhiteboard(Setting::Ptr<>) const override;
    };
}

/************************************************************************/
/*
 * ToDo: this is pretty much messed up. It works, as far as I can
 * tell, but it's not nice...
 */

namespace SteamBot
{
    class ClientInfo
    {
    private:
        bool active=false;
        std::shared_ptr<Client> client;

    private:
        std::string displayName(const ClientInfo*) const;

    public:
        const std::string accountName;

        std::string displayName() const;

    public:
        ClientInfo(std::string);		// internal use
        ~ClientInfo();

    public:
        // internal use
        bool setActive(bool);
        void setClient(std::shared_ptr<Client>);

    public:
        bool isActive() const;
        std::shared_ptr<Client> getClient() const;

        static void init();

        static ClientInfo* create(std::string);
        static ClientInfo* find(std::string_view);
        static std::vector<ClientInfo*> findDisplay(std::string_view);	// display name

        static ClientInfo* find(std::function<bool(const boost::json::value&)>);
        static ClientInfo* find(SteamBot::AccountID);

        static std::string prettyName(SteamBot::AccountID);

    public:
        static std::vector<ClientInfo*> getClients();
        static std::vector<ClientInfo*> getGroup(std::string_view);

        static void quitAll();
    };
}

