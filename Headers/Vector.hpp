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

#include <cstddef>
#include <vector>

/************************************************************************/
/*
 * Erase items from a vector by moving the last item into the
 * erased positions. This means the ordering changes.
 */

/************************************************************************/

namespace SteamBot
{
    template <typename T, typename PRED> size_t erase(std::vector<T>& vector, PRED pred)
    {
        size_t count=0;
        size_t i=0;
        while (i<vector.size())
        {
            if (pred(vector[i]))
            {
                if (i+1<vector.size())
                {
                    vector[i]=std::move(vector.back());
                }
                vector.pop_back();
                count++;
            }
            else
            {
                i++;
            }
        }
        return count;
    }
}
