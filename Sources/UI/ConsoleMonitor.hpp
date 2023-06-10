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
 * This is for the console UI, and currently only implemented for
 * Linux.
 *
 * It monitors the stdin for the user hitting the command key (return
 * or tab) indicating he wishes to enter CLI commands. It will
 * call the provided callback to inform you about this event.
 *
 * The console code can turn this on/off as needed -- like it will
 * turn it off when the user is expected to input a password.
 */

#include <memory>
#include <functional>

/************************************************************************/

namespace SteamBot
{
    namespace UI
    {
        class ConsoleMonitorBase
        {
        public:
            typedef std::function<void()> Callback;

        protected:
            Callback callback;

        public:
            virtual void enable() =0;
            virtual void disable() =0;

        protected:
            ConsoleMonitorBase(Callback&& callback_)
                : callback(std::move(callback_))
            {
            }

        public:
            virtual ~ConsoleMonitorBase() =default;

        public:
            static std::unique_ptr<ConsoleMonitorBase> create(Callback);
        };
    }
}
