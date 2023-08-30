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

#include "Helpers/StringCompare.hpp"

/************************************************************************/

std::weak_ordering SteamBot::caseInsensitiveStringCompare(std::string_view left, std::string_view right)
{
    return std::lexicographical_compare_three_way(left.begin(), left.end(), right.begin(), right.end(),
                                                  [](char cLeft, char cRight){
                                                      if (cLeft>='A' && cLeft<='Z') cLeft+=('a'-'A');
                                                      if (cRight>='A' && cRight<='Z') cRight+=('a'-'A');
                                                      return cLeft<=>cRight;
                                                  });
}
