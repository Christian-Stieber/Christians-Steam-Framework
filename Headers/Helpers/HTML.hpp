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

#include <string>

/************************************************************************/

namespace HTMLParser
{
    namespace Tree
    {
        class Element;
    }
}

/************************************************************************/
/*
 * Checks whether a class name is contained in a class string (space
 * separated list of class names).
 *
 * The Element version gets the class attribute to check.
 */

namespace SteamBot
{
    namespace HTML
    {
        bool checkClass(std::string_view, const char*);
        bool checkClass(const HTMLParser::Tree::Element&, const char*);
    }
}

/************************************************************************/
/*
 * Get "clean text" from an element.
 *
 * This does the following things:
 *   - recursively collect and concatenate all text nodes
 *   - cleanup whitespace
 */

namespace SteamBot
{
    namespace HTML
    {
        std::string getCleanText(const HTMLParser::Tree::Element&);
    }
}
