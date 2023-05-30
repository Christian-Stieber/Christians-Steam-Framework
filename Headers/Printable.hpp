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

#include <boost/json.hpp>

#include <iostream>

/************************************************************************/
/*
 * This is an attempt to help make standardized printable objects.
 *
 * The JSON produced here is meant for output, so it need not be able
 * to allow for a roundtrip conversion.
 */

namespace SteamBot
{
    class Printable
    {
    protected:
        Printable() =default;

    public:
        virtual ~Printable() =default;
        virtual boost::json::value toJson() const =0;
    };

    inline std::ostream& operator<<(std::ostream& stream, const Printable& printable)
    {
        return stream << printable.toJson();
    }
}
