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

#include <memory>

/************************************************************************/
/*
 * When used as a base class, lets you get a callback when the object
 * is destructued.
 */

namespace SteamBot
{
    class DestructMonitor
    {
    public:
        class DestructCallback
        {
        public:
            DestructCallback() =default;
            virtual ~DestructCallback() =default;
            virtual void call(const DestructMonitor*) =0;
        };

    public:
        std::weak_ptr<DestructCallback> destructCallback;

    public:
        DestructMonitor();
        virtual ~DestructMonitor();
    };
}
