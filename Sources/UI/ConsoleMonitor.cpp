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

#include "./ConsoleMonitor.hpp"

#include <mutex>
#include <condition_variable>
#include <cassert>

#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <poll.h>

/************************************************************************/

namespace
{
    // https://stackoverflow.com/questions/12171377/how-to-convert-errno-to-exception-using-system-error
    void throwErrno(int errno_=errno)
    {
        throw std::system_error(errno_, std::generic_category());
    }
}

/************************************************************************/
/*
 * https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
 */

namespace
{
    class RawInput
    {
    private:
        int fd;
        struct termios saved;

    public:
        RawInput(int fd_)
            : fd(fd_)
        {
            if (tcgetattr(fd, &saved)==0)
            {
                auto changed=saved;
                changed.c_lflag &= ~ICANON;
                changed.c_lflag &= ~ECHO;
                changed.c_cc[VMIN] = 1;
                changed.c_cc[VTIME] = 0;
                if (tcsetattr(fd, TCSANOW, &changed)==0)
                {
                    return;
                }
            }
            throwErrno();
        }

        ~RawInput()
        {
            if (tcsetattr(fd, TCSANOW, &saved)<0)
            {
                throwErrno();
            }
        }
    };
}

/************************************************************************/

namespace
{
    class ConsoleMonitor : public SteamBot::UI::ConsoleMonitorBase
    {
    private:
        static inline constexpr int signalNumber=SIGUSR1;

        std::thread thread;

        enum class Command : uint8_t {
            Startup,
            Confirmed,
            Monitor,
            NoMonitor
        } command=Command::Startup;

        std::mutex mutex;
        std::condition_variable condition;

    private:
        // these are called from the UI thread
        void sendCommand(Command);

        virtual void enable() override
        {
            sendCommand(Command::Monitor);
        }

        virtual void disable() override
        {
            sendCommand(Command::NoMonitor);
        }

    private:
        // these are used on our thread
        void body();

    public:
        ConsoleMonitor(Callback&& callback_)
            : ConsoleMonitorBase(std::move(callback_))
        {
            thread=std::thread([this](){
                body();
            });

            std::unique_lock<decltype(mutex)> lock(mutex);
            condition.wait(lock, [this](){ return command==Command::Confirmed; });
        }

        virtual ~ConsoleMonitor() =default;
    };
}

/************************************************************************/

void ConsoleMonitor::sendCommand(ConsoleMonitor::Command command_)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        assert(command==Command::Confirmed);
        command=command_;
    }
    condition.notify_one();

    if (auto error=pthread_kill(thread.native_handle(), signalNumber)) throwErrno(error);

    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this]() { return command==Command::Confirmed; });
    assert(command==Command::Confirmed);
}

/************************************************************************/

volatile bool signalReceived=false;

static void dummyHandler(int)
{
    signalReceived=true;
}

/************************************************************************/
/*
 * Signal stuff: https://lwn.net/Articles/176911/
 */

void ConsoleMonitor::body()
{
    int fd=fileno(stdin);
    std::unique_ptr<RawInput> rawInput;

    sigset_t blockset;
    sigemptyset(&blockset);
    sigaddset(&blockset, signalNumber);
    pthread_sigmask(SIG_BLOCK, &blockset, nullptr);

    struct sigaction sa;
    sa.sa_sigaction=nullptr;
    sa.sa_handler=&dummyHandler;
    sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
    sigaction(signalNumber, &sa, nullptr);

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        assert(command==Command::Startup);
        command=Command::Confirmed;
    }
    condition.notify_one();

    sigset_t emptyset;
    sigemptyset(&emptyset);

    bool commandKey=false;
    while (true)
    {
        if (!commandKey && rawInput)
        {
            struct pollfd pfd;
            pfd.fd=fd;
            pfd.events=POLLIN;

            if (ppoll(&pfd, 1, nullptr, &emptyset)<0 && errno!=EINTR) throwErrno();

            if (signalReceived)
            {
                signalReceived=false;
            }
            else if ((pfd.revents & (POLLERR | POLLHUP | POLLIN)))
            {
                char c;
                switch(read(fd, &c, 1))
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

                defaulr:
                    throwErrno();
                    break;
                }
            }
        }
        else
        {
            std::unique_lock<decltype(mutex)> lock(mutex);
            condition.wait(lock, [this](){ return command!=Command::Confirmed; });
        }

        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            switch(command)
            {
            case Command::Monitor:
                if (!rawInput)
                {
                    rawInput=std::make_unique<RawInput>(fd);
                    commandKey=false;
                }
                break;

            case Command::NoMonitor:
                rawInput.reset();
                commandKey=false;
                break;

            case Command::Confirmed:
                break;

            default:
                assert(false);
            }
            command=Command::Confirmed;
        }
        condition.notify_one();

        if (commandKey && rawInput)
        {
            callback();
        }
    }
}

/************************************************************************/

std::unique_ptr<SteamBot::UI::ConsoleMonitorBase> SteamBot::UI::ConsoleMonitorBase::create(SteamBot::UI::ConsoleMonitorBase::Callback callback)
{
    return std::make_unique<ConsoleMonitor>(std::move(callback));
}

/************************************************************************/

#endif /* __linux__ */
