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

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace DiscoveryQueue
        {
            void use();

            namespace Messageboard
            {
                // Send this to trigger a queue clearing.
                // Note that your message might be ignored if clearing
                // is already in progress; do not expect a
                // QueueCompleted for every ClearQueue.
                class ClearQueue
                {
                public:
                    ClearQueue();
                    ~ClearQueue();
                };

                // Sent when a queue has been cleared
                class QueueCompleted : public std::chrono::system_clock::time_point
                {
                public:
                    QueueCompleted();
                    ~QueueCompleted();
                };
            }
        }
    }
}
