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

#include "Asio/Asio.hpp"
#include "Asio/Signals.hpp"
#include "UI/UI.hpp"
#include "Client/Client.hpp"

#include <boost/asio/signal_set.hpp>

/************************************************************************/

namespace
{
    class SignalHandler
    {
    private:
        boost::asio::signal_set signals;
        bool terminating=false;

    public:
        SignalHandler(boost::asio::io_context&);
        ~SignalHandler() =delete;

    private:
        void waitSignal();
    };
}

/************************************************************************/
/*
 * Threads inherit signal masks from their parent, so by blocking them
 * on the main thread, we should be able to just unblock them on the
 * asio thread and everything will arrive there.
 */

static void blockSignals()
{
    sigset_t signals;
    if (sigemptyset(&signals)==0 &&
        sigaddset(&signals, SIGINT)==0 &&
        sigaddset(&signals, SIGTERM)==0 &&
        pthread_sigmask(SIG_BLOCK, &signals, nullptr)==0)
    {
    }
    else
    {
        throw std::runtime_error("unable to configure signals");
    }
}

/************************************************************************/

void SignalHandler::waitSignal()
{
    signals.async_wait([this](const boost::system::error_code& error, int signalNumber) {
        if (error)
        {
            BOOST_LOG_TRIVIAL(error) << "signal handler has returned an error: " << error;
        }
        else
        {
            BOOST_LOG_TRIVIAL(info) << "received a signal: " << signalNumber;
            assert(signalNumber==SIGINT || signalNumber==SIGTERM);

            if (terminating)
            {
                // emergency quit
                signals.clear();
                raise(signalNumber);
            }
            else
            {
                terminating=true;
                std::thread([](){
                    SteamBot::ClientInfo::quitAll();
                    SteamBot::UI::Thread::quit();
                }).detach();
            }
        }
        waitSignal();
    });
}

/************************************************************************/

SignalHandler::SignalHandler(boost::asio::io_context& ioContext)
    : signals(ioContext, SIGINT, SIGTERM)
{
    waitSignal();
}

/************************************************************************/

void SteamBot::handleSignals(boost::asio::io_context& ioContext)
{
    [[maybe_unused]] static SignalHandler& handler=*new SignalHandler(ioContext);
}

/************************************************************************/

void SteamBot::initSignals()
{
    SteamBot::Asio::getIoContext();
    blockSignals();
}
