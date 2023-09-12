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

/************************************************************************/
/*
 * This module maintains "last login" data in the account data file,
 * containing the time of the login, and how long we were logged in.
 */

/************************************************************************/

#include "Modules/Login.hpp"
#include "Client/Module.hpp"
#include "Helpers/JSON.hpp"
#include "Exceptions.hpp"
#include "Modules/LoginTracking.hpp"

#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

typedef SteamBot::Modules::Login::Whiteboard::LoginStatus LoginStatus;

/************************************************************************/

namespace
{
    struct Keys
    {
        inline static const std::string_view lastLogin="Last login";
        inline static const std::string_view when="when";
        inline static const std::string_view duration="duration";
    };
}

/************************************************************************/

namespace
{
    class LoginTrackingModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<LoginStatus> loginStatus;
        std::chrono::system_clock::time_point loginTime;

    private:
        void update(SteamBot::Client&) const;

    public:
        LoginTrackingModule() =default;
        virtual ~LoginTrackingModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    LoginTrackingModule::Init<LoginTrackingModule> init;
}

/************************************************************************/

void LoginTrackingModule::init(SteamBot::Client& client)
{
    loginStatus=client.whiteboard.createWaiter<LoginStatus>(*waiter);
}

/************************************************************************/
/*
 * Check login status, and maybe update the data in the file
 */

void LoginTrackingModule::update(SteamBot::Client& client) const
{
    if (loginTime.time_since_epoch().count()!=0)
    {
        client.dataFile.update([this](boost::json::value& json) {
            auto& item=SteamBot::JSON::createItem(json, Keys::lastLogin).emplace_object();
            {
                item[Keys::when]=std::chrono::system_clock::to_time_t(loginTime);

                std::chrono::seconds duration=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()-loginTime);
                item[Keys::duration]=duration.count();
            }
            return true;
        });
    }
}

/************************************************************************/

void LoginTrackingModule::run(SteamBot::Client& client)
{
    try
    {
        while (true)
        {
            waiter->wait();
            switch(loginStatus->get(LoginStatus::LoggedOut))
            {
            case LoginStatus::LoggedOut:
                update(client);
                loginTime=decltype(loginTime)();
                break;

            case LoginStatus::LoggingIn:
                assert(loginTime.time_since_epoch().count()==0);
                break;

            case LoginStatus::LoggedIn:
                if (loginTime.time_since_epoch().count()==0)
                {
                    loginTime=std::chrono::system_clock::now();
                    update(client);
                }
                break;

            default:
                assert(false);
            }
        }
    }
    catch(const SteamBot::OperationCancelledException&)
    {
        update(client);
    }
}

/************************************************************************/

bool SteamBot::Modules::LoginTracking::TrackingData::get(const boost::json::value& json)
{
    try
    {
        if (auto item=SteamBot::JSON::getItem(json, Keys::lastLogin))
        {
            if (auto whenItem=SteamBot::JSON::getItem(*item, Keys::when))
            {
                when=std::chrono::system_clock::from_time_t(SteamBot::JSON::toNumber<std::time_t>(*whenItem));
                if (auto durationItem=SteamBot::JSON::getItem(*item, Keys::duration))
                {
                    duration=std::chrono::seconds(SteamBot::JSON::toNumber<decltype(duration)::rep>(*durationItem));
                    return true;
                }
            }
        }
        }
    catch(...)
    {
        BOOST_LOG_TRIVIAL(error) << "dataFile exception " << boost::current_exception_diagnostic_information();
    }
    return false;
}

/************************************************************************/

bool SteamBot::Modules::LoginTracking::TrackingData::get(DataFile& dataFile)
{
    return dataFile.examine([this](const boost::json::value& json) {
        return get(json);
    });
}

/************************************************************************/

SteamBot::Modules::LoginTracking::TrackingData::TrackingData() =default;
SteamBot::Modules::LoginTracking::TrackingData::~TrackingData() =default;
