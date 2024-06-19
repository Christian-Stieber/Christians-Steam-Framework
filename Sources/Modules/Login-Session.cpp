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
 * I have no idea yet how to correctly handle authentication with
 * SteamGuard codes -- timeouts are rather short, and heartbeats or
 * auth status polling did anything.
 *
 * So, I'm now doing the interactive stuff outside of the actual
 * client login module, and launch a client from here when I have
 * obtained data from the user to try and progress the login.
 *
 * For each query (password and guard code), I'm completely shutting
 * down the client, and start another one when the user has provided
 * some string to try.
 */

#include "Login-Session.hpp"
#include "UI/UI.hpp"
#include "Client/Signal.hpp"

#include <boost/fiber/protected_fixedsize_stack.hpp>

/************************************************************************/

typedef SteamBot::Modules::Login::Internal::CredentialsSession CredentialsSession;

/************************************************************************/
/*
 * An instance of this is on the SessionThread when we are asking
 * the user for something.
 */

namespace
{
    class SessionState
    {
    private:
        std::shared_ptr<CredentialsSession> session;
        SteamBot::UI::Base::ResultParam<std::string> input;
        SteamBot::UI::Base::PasswordType passwordType;

    public:
        SessionState(std::shared_ptr<CredentialsSession> session_, SteamBot::UI::Base::PasswordType passwordType_, std::shared_ptr<SteamBot::Waiter> waiter)
            : session(std::move(session_)),
              input(SteamBot::UI::Thread::requestPassword(waiter, passwordType_)),
              passwordType(passwordType_)
        {
        }

        ~SessionState()
        {
            input->cancel();
        }

    public:
        bool handle()
        {
            assert(input);
            if (auto string=input->getResult())
            {
                switch(passwordType)
                {
                case SteamBot::UI::Base::PasswordType::AccountPassword:
                    session->password=*string;
                    break;

                case SteamBot::UI::Base::PasswordType::SteamGuard_EMail:
                    session->steamGuardCode=*string;
                    break;

                default:
                    assert(false);
                }

                SteamBot::Client::launch(session->clientInfo);
                return true;
            }
            return false;
        }
    };
}

/************************************************************************/
/*
 * The Session thread has all the sessions that are currently
 * asking information from the user.
 */

namespace
{
    class SessionThread
    {
    private:
        const std::shared_ptr<SteamBot::Waiter> waiter{SteamBot::Waiter::create()};
        SteamBot::Signal signal{waiter};

        std::vector<SessionState> sessions;

        boost::fibers::mutex mutex;
        boost::fibers::condition_variable condition;
        std::queue<std::shared_ptr<CredentialsSession>> threadQueue;
        bool running=false;

    private:
        SessionThread() =default;
        ~SessionThread() =delete;

    private:
        void handleInputs()
        {
            SteamBot::erase(sessions, [](SessionState& state) {
                return state.handle();
            });
        }

    private:
        void launchSessions(std::queue<std::shared_ptr<CredentialsSession>>& fiberQueue)
        {
            while (true)
            {
                std::shared_ptr<CredentialsSession> session;
                {
                    std::lock_guard<decltype(mutex)> lock(mutex);
                    if (fiberQueue.empty())
                    {
                        return;
                    }
                    session=std::move(fiberQueue.front());
                    fiberQueue.pop();
                }

                if (session->password.empty())
                {
                    sessions.emplace_back(std::move(session), SteamBot::UI::Base::PasswordType::AccountPassword, waiter);
                }
                else if (session->steamGuardCode.empty())
                {
                    sessions.emplace_back(std::move(session), SteamBot::UI::Base::PasswordType::SteamGuard_EMail, waiter);
                }
                else
                {
                    assert(false);
                }
            }
        }

    private:
        void body()
        {
            bool quit=false;
            std::queue<std::shared_ptr<CredentialsSession>> fiberQueue;

            auto mainFiber=boost::fibers::fiber(std::allocator_arg, boost::fibers::protected_fixedsize_stack(), [this, &quit, &fiberQueue]() {
                while (true)
                {
                    waiter->wait();
                    launchSessions(fiberQueue);
                    handleInputs();

                    {
                        std::lock_guard<decltype(mutex)> lock(mutex);
                        if (threadQueue.empty() && fiberQueue.empty() && sessions.empty())
                        {
                            running=false;
                            quit=true;
                        }
                    }

                    if (quit)
                    {
                        condition.notify_one();
                        return;
                    }
                }
            });

            auto commFiber=boost::fibers::fiber(std::allocator_arg, boost::fibers::protected_fixedsize_stack(), [this, &quit, &fiberQueue]() {
                while (true)
                {
                    std::unique_lock<decltype(mutex)> lock(mutex);
                    condition.wait(lock, [this, &quit](){
                        return quit || !threadQueue.empty();
                    });
                    if (quit) return;
                    while (!threadQueue.empty())
                    {
                        fiberQueue.push(std::move(threadQueue.front()));
                        threadQueue.pop();
                    }
                    signal.signal();
                }
            });

            mainFiber.join();
            commFiber.join();
        }

    public:
        static SessionThread& get()
        {
            static SessionThread& instance=*new SessionThread();
            return instance;
        }

    public:
        void run(std::shared_ptr<CredentialsSession> session)
        {
            {
                std::lock_guard<decltype(mutex)> lock(mutex);
                threadQueue.push(std::move(session));
                if (!running)
                {
                    std::thread([this]() { body(); }).detach();
                    running=true;
                }
            }
            condition.notify_one();
        }
    };
}

/************************************************************************/

CredentialsSession::CredentialsSession(SteamBot::ClientInfo& clientInfo_)
    : clientInfo(clientInfo_)
{
}

/************************************************************************/

CredentialsSession::~CredentialsSession() =default;

/************************************************************************/
/*
 * Get the session for the clientInfo. Create one if we don't have one.
 */

CredentialsSession& CredentialsSession::get(SteamBot::ClientInfo& clientInfo)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    for (const auto& session: sessions)
    {
        if (&session->clientInfo==&clientInfo)
        {
            return *session;
        }
    }
    return *sessions.emplace_back(std::make_unique<CredentialsSession>(clientInfo));
}

/************************************************************************/
/*
 * This must be called from the client. It terminates the client,
 * asks the user for whatever we need, and restarts the client.
 */

void CredentialsSession::run()
{
    SteamBot::Client::getClient().quit();
    SessionThread::get().run(shared_from_this());
}
