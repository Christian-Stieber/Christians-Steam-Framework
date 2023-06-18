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

#ifdef _WIN32

/************************************************************************/

#include "./Console.hpp"
#include "EnumString.hpp"

#include <Windows.h>
#include <synchapi.h>

/************************************************************************/

typedef SteamBot::UI::ConsoleUI::Manager Manager;

/************************************************************************/

namespace
{
	class Event
	{
	private:
		HANDLE event;

	public:
		Event()
		{
			event=CreateEventW(NULL, FALSE, FALSE, NULL);
			if (event==NULL)
			{
				throw std::runtime_error("CreateEventW() failed");
			}
		}

		~Event()
		{
			if (event!=NULL)
			{
				CloseHandle(event);
			}
		}

	public:
		void signal()
		{
			if (!SetEvent(event))
			{
				throw std::runtime_error("SetSignal() failed");
			}
		}

	public:
		HANDLE& getHandle()
		{
			return event;
		}
	};
}

/************************************************************************/

class SteamBot::UI::ConsoleUI::Manager : public SteamBot::UI::ConsoleUI::ManagerBase
{
private:
	std::thread thread;

	std::mutex mutex;
	std::condition_variable condition;

	Event event;

    Mode newMode=Mode::Startup;

private:
    virtual void setMode(Mode) override;
	void body();

public:
    Manager(SteamBot::UI::ConsoleUI&);
    virtual ~Manager();
};

/************************************************************************/

void Manager::body()
{
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		newMode=Mode::None;
	}
	condition.notify_one();

	const HANDLE consoleInputHandle=GetStdHandle(STD_INPUT_HANDLE);
	bool commandKey=false;
    Mode mode=Mode::LineInput;
    while (mode!=Mode::Shutdown)
	{
        BOOST_LOG_TRIVIAL(debug) << "Console manager: current mode is " << SteamBot::enumToString(mode) << "; commandKey " << commandKey;

		DWORD waitResult;
		{
			DWORD handleCount=1;
			HANDLE handles[2];
			handles[0]=event.getHandle();
			if (mode==Mode::NoInput && !commandKey)
			{
				handles[1]=consoleInputHandle;
				handleCount=2;
			}
			waitResult=WaitForMultipleObjects(handleCount, handles, FALSE, INFINITE);
		}
		if (waitResult==WAIT_FAILED)
		{
			throw std::runtime_error("WaitForMultipleObjects() failed");
		}

		// First, check for "mode" changes
		{
			std::lock_guard<decltype(mutex)> lock(mutex);
			switch(newMode)
			{
			case Mode::None:
				break;

			case Mode::NoInput:
			case Mode::LineInput:
			case Mode::LineInputNoEcho:
			case Mode::Shutdown:
				mode=newMode;
				commandKey=false;
				break;

			default:
				assert(false);
			}
			newMode=Mode::None;
        }

		if (waitResult==WAIT_OBJECT_0+0)
		{
			// Event
			condition.notify_one();
		}
		else if (waitResult==WAIT_OBJECT_0+1)
		{
			// stdin
			if (mode==Mode::NoInput)
			{
				DWORD count;
				INPUT_RECORD inputRecord;
				if (ReadConsoleInputW(consoleInputHandle, &inputRecord, 1, &count)==0)
				{
					throw std::runtime_error("ReadConsoleInputW() failed");
				}
				assert(count==1);
				if (count==1)
				{
					if (inputRecord.EventType==KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown==FALSE)
					{
						switch (inputRecord.Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_TAB:
						case VK_RETURN:
							commandKey=true;
							executeOnThread([this](){ ui.performCli(); });
							break;
						}
					}
				}
			}
		}
		else
		{
			throw std::runtime_error("WaitForMultipleObjects() returned unexpected value");
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

	event.signal();

    std::unique_lock<decltype(mutex)> lock(mutex);
    condition.wait(lock, [this]() { return newMode==Mode::None; });
}

/************************************************************************/

std::unique_ptr<SteamBot::UI::ConsoleUI::ManagerBase> SteamBot::UI::ConsoleUI::ManagerBase::create(SteamBot::UI::ConsoleUI& ui)
{
    return std::make_unique<Manager>(ui);
}

/************************************************************************/

#endif /* _WIN32 */
