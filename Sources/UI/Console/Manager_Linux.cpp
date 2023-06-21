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

#ifdef __linux__

/************************************************************************/

#include "./Console.hpp"
#include "EnumString.hpp"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>

#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

/************************************************************************/

typedef SteamBot::UI::ConsoleUI::Manager Manager;

/************************************************************************/

class SteamBot::UI::ConsoleUI::Manager : public SteamBot::UI::ConsoleUI::ManagerBase
{
private:
    static constexpr int signalNumber=SIGUSR1;

private:
    static void throwErrno(int errno_=errno);
    static int getFd();

private:
    class SavedTermios;

private:
    static inline std::thread thread;

    static inline std::mutex mutex;
    static inline std::condition_variable condition;
    static inline Mode newMode=Mode::Startup;

private:
    class SignalHandler
    {
    private:
        static inline bool received=false;

    public:
        static void handler(int number);
        static void install();
        static bool checkAndReset();
    };

private:
    virtual void setMode(Mode) override;
    void body();

public:
    Manager(SteamBot::UI::ConsoleUI&);
    virtual ~Manager();
};

/************************************************************************/
/*
 * https://stackoverflow.com/questions/12171377/how-to-convert-errno-to-exception-using-system-error
 */

void Manager::throwErrno(int errno_)
{
    throw std::system_error(errno_, std::generic_category());
}

/************************************************************************/

int Manager::getFd()
{
    static const int fd=fileno(stdin);
    return fd;
}

/************************************************************************/

void Manager::SignalHandler::handler(int number)
{
    assert(number==signalNumber);
    received=true;
}

/************************************************************************/

void Manager::SignalHandler::install()
{
    sigset_t blockset;
    sigemptyset(&blockset);
    sigaddset(&blockset, signalNumber);
    pthread_sigmask(SIG_BLOCK, &blockset, nullptr);

    struct sigaction sa;
    sa.sa_sigaction=nullptr;
    sa.sa_handler=&handler;
    sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
    sigaction(signalNumber, &sa, nullptr);
}

/************************************************************************/

bool Manager::SignalHandler::checkAndReset()
{
    bool result=received;
    received=false;
    return result;
}

/************************************************************************/
/*
 * https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
 */

class Manager::SavedTermios
{
private:
    static inline bool isSaved=false;
    static inline struct termios saved;

public:
    SavedTermios(Mode mode)
    {
        assert(!isSaved);

        int fd=getFd();
        if (tcgetattr(fd, &saved)==0)
        {
            isSaved=true;

            auto changed=saved;
            switch(mode)
            {
            case Mode::NoInput:
                changed.c_cc[VMIN] = 1;
                changed.c_cc[VTIME] = 0;
                changed.c_lflag &= ~(ICANON | ECHO);
                break;

            case Mode::LineInputNoEcho:
                changed.c_lflag &= ~(ECHO);

            default:
                assert(false);
            }
            if (tcsetattr(fd, TCSANOW, &changed)==0)
            {
                return;
            }
        }
        throwErrno();
    }

    ~SavedTermios()
    {
        if (isSaved)
        {
            isSaved=false;
            if (tcsetattr(getFd(), TCSANOW, &saved)<0)
            {
                throwErrno();
            }
        }
    }
};

/************************************************************************/
/*
 * Signal stuff: https://lwn.net/Articles/176911/
 */

void Manager::body()
{
    SignalHandler::install();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        assert(newMode==Mode::Startup);
        newMode=Mode::None;
    }
    condition.notify_one();

    sigset_t emptyset;
    sigemptyset(&emptyset);

    std::unique_ptr<SavedTermios> savedTermios;
    bool commandKey=false;
    Mode mode=Mode::LineInput;
    while (mode!=Mode::Shutdown)
    {
        BOOST_LOG_TRIVIAL(debug) << "Console manager: current mode is " << SteamBot::enumToString(mode) << "; commandKey " << commandKey;

        struct pollfd pfd;
        pfd.events=POLLIN;
        if (mode==Mode::NoInput && !commandKey)
        {
            pfd.fd=getFd();
        }
        else
        {
            pfd.fd=-1;
        }

        if (ppoll(&pfd, 1, nullptr, &emptyset)<0)
        {
            if (errno!=EINTR)
            {
                throwErrno();
            }
        }

        if (SignalHandler::checkAndReset())
        {
        }
        else if ((pfd.revents & (POLLERR | POLLHUP | POLLIN)))
        {
            char c;
            switch(read(pfd.fd, &c, 1))
            {
            case 0:
                throwErrno(0);
                break;

            case 1:
                if (c=='\n' || c=='\t')
                {
                    commandKey=true;
                }
                break;

            default:
                throwErrno();
                break;
            }
        }

        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            switch(newMode)
            {
            case Mode::NoInput:
                savedTermios.reset();
                savedTermios=std::make_unique<SavedTermios>(Mode::NoInput);
                mode=Mode::NoInput;
                commandKey=false;
                break;

            case Mode::LineInput:
                savedTermios.reset();
                mode=Mode::LineInput;
                commandKey=false;
                break;

            case Mode::LineInputNoEcho:
                savedTermios.reset();
                savedTermios=std::make_unique<SavedTermios>(Mode::LineInputNoEcho);
                mode=Mode::LineInputNoEcho;
                commandKey=false;
                break;

            case Mode::None:
                break;

            case Mode::Shutdown:
                mode=Mode::Shutdown;
                break;

            default:
                assert(false);
            }
            newMode=Mode::None;
        }
        condition.notify_one();

        if (commandKey && mode==Mode::NoInput)
        {
            executeOnThread([this](){ ui.performCli(); });
        }
    }
}

/************************************************************************/

Manager::Manager(SteamBot::UI::ConsoleUI& ui_)
    : ManagerBase(ui_)
{
    thread=std::thread([this](){
        BOOST_LOG_TRIVIAL(debug) << "Console manager thread running";
        body();
        BOOST_LOG_TRIVIAL(debug) << "Console manager thread exiting";
    });

    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this](){ return newMode==Mode::None; });
}

/************************************************************************/

Manager::~Manager()
{
    setMode(Mode::Shutdown);
    thread.join();
}

/************************************************************************/

void Manager::setMode(Mode mode)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        assert(newMode==Mode::None);
        newMode=mode;
    }

    if (auto error=pthread_kill(thread.native_handle(), signalNumber))
    {
        throwErrno(error);
    }

    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this]() { return newMode==Mode::None; });
}

/************************************************************************/

std::unique_ptr<SteamBot::UI::ConsoleUI::ManagerBase> SteamBot::UI::ConsoleUI::ManagerBase::create(SteamBot::UI::ConsoleUI& ui)
{
    return std::make_unique<Manager>(ui);
}

/************************************************************************/

#endif /* __linux__ */
