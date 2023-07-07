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
 * For conteiners that have std::shared_ptr<>/std::unique_ptr<>.
 *
 * "T" is the smart-pointer type.
 */

namespace SteamBot
{
    template <typename T> struct SmartDerefStuff
    {
        struct Hash
        {
            std::size_t operator() (const T& item) const
            {
                return std::hash<std::remove_cv_t<typename T::element_type>>()(*item);
            }
        };

        struct Equals
        {
            size_t operator()(const T& left, const T& right) const
            {
                return *left==*right;
            }
        };
    };
}
