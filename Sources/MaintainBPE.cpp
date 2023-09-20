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

#include "MaintainBPE.hpp"
#include "Client/Client.hpp"
#include "Modules/LoginTracking.hpp"
#include "Modules/Executor.hpp"
#include "Helpers/Time.hpp"
#include "Asio/Asio.hpp"

#include <boost/asio/system_timer.hpp>

/************************************************************************/
/*
 * ToDo: This module is a big mess -- error handling, waiting for
 * completion, interacting with other activities etc. All broken.
 */

/************************************************************************/
/*
 * ToDo: we need to handle the case of a new account getting created.
 *
 * This really only matters if we have no accounts (since otherwise
 * the new account will always be newer anyway).
 */

/************************************************************************/
/*
 * We must have a login every 7 days to maintain booster pack
 * eligibilty, but let's play it safe.
 */

static std::chrono::days maxPause(5);

/************************************************************************/
/*
 * Find the (or "an" if there are multiple) account with the oldest login
 */

namespace
{
    class OldestLogin
    {
    public:
        SteamBot::ClientInfo* clientInfo=nullptr;
        std::chrono::system_clock::time_point logoff;

    public:
        boost::json::value toJson() const
        {
            boost::json::object object;
            if (clientInfo!=nullptr)
            {
                object["account"]=clientInfo->accountName;
                object["logoff"]=SteamBot::Time::toString(logoff);
            }
            return object;
        }

    public:
        OldestLogin()
        {
            for (auto info : SteamBot::ClientInfo::getClients())
            {
                SteamBot::Modules::LoginTracking::TrackingData data;
                if (data.get(SteamBot::DataFile::get(info->accountName, SteamBot::DataFile::FileType::Account)))
                {
                    const decltype(logoff) timestamp=data.when+data.duration;
                    if (clientInfo==nullptr || timestamp<logoff)
                    {
                        logoff=timestamp;
                        clientInfo=info;
                    }
                }
            }
            BOOST_LOG_TRIVIAL(info) << "oldest login: " << toJson();
        }
    };
}

/************************************************************************/

namespace
{
    class MaintainBPE
    {
    private:
        boost::asio::system_timer timer;

    private:
        void clientReady(SteamBot::ClientInfo*);
        void performLogin(SteamBot::ClientInfo*);
        void startTimer();

    public:
        MaintainBPE()
            : timer(SteamBot::Asio::getIoContext())
        {
            startTimer();
        }
    };
}

/************************************************************************/

void MaintainBPE::clientReady(SteamBot::ClientInfo* clientInfo)
{
    SteamBot::Modules::Executor::executeWithFiber(clientInfo->getClient(), [this](SteamBot::Client& client) {
        SteamBot::Client::waitForLogin();
        boost::this_fiber::sleep_for(std::chrono::seconds(5));
        client.quit();

        boost::this_fiber::sleep_for(std::chrono::seconds(1));
        SteamBot::Asio::post("MaintainBPE::startTimer", [this](){
            startTimer();
        });
    });
}

/************************************************************************/

void MaintainBPE::performLogin(SteamBot::ClientInfo* clientInfo)
{
    BOOST_LOG_TRIVIAL(info) << "MaintainBPE: login \"" << clientInfo->accountName << "\"";

    class WaitClient : public std::enable_shared_from_this<WaitClient>
    {
    private:
        MaintainBPE& maintainBPE;
        SteamBot::ClientInfo* clientInfo;

    public:
        WaitClient(MaintainBPE& maintainBPE_, SteamBot::ClientInfo* clientInfo_)
            : maintainBPE(maintainBPE_),
              clientInfo(clientInfo_)
        {
        }

    public:
        void wait()
        {
            maintainBPE.timer.expires_after(std::chrono::seconds(1));
            maintainBPE.timer.async_wait([self=shared_from_this()](const boost::system::error_code& error){
                if (error)
                {
                    // ToDo?
                    throw error;
                }
                else
                {
                    if (self->clientInfo->getClient())
                    {
                        self->maintainBPE.clientReady(self->clientInfo);
                    }
                    else
                    {
                        self->wait();
                    }
                }
            });
        }
    };

    SteamBot::Client::launch(*clientInfo);
    auto wait=std::make_shared<WaitClient>(*this, clientInfo);
    wait->wait();
}

/************************************************************************/

void MaintainBPE::startTimer()
{
    assert(SteamBot::Asio::isThread());
    const OldestLogin oldestLogin;
    if (oldestLogin.clientInfo!=nullptr)
    {
        auto interval=oldestLogin.logoff+maxPause-std::chrono::system_clock::now();
        if (interval.count()>0)
        {
            timer.expires_after(interval);
            BOOST_LOG_TRIVIAL(info) << "setting MaintainBPE-timer for " << SteamBot::Time::toString(timer.expiry());
            timer.async_wait([this](const boost::system::error_code& error){
                if (error)
                {
                    BOOST_LOG_TRIVIAL(error) << "MaintainBPE-timer returned error: " << error;
                    throw error;
                }
                else
                {
                    startTimer();
                }
            });
        }
        else
        {
            performLogin(oldestLogin.clientInfo);
        }
    }
    else
    {
        // ToDo: what to do?
    }
}

/************************************************************************/

void SteamBot::MaintainBoosterPackEligibility::run()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        SteamBot::Asio::post("MaintainBPE", [](){
            [[maybe_unused]] static MaintainBPE& _=*new MaintainBPE;
        });
    });
}
