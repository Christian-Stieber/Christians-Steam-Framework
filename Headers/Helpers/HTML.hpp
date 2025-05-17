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
#include <optional>
#include <functional>

#include "Helpers/ParseNumber.hpp"
#include "HTMLParser/Tree.hpp"

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

/************************************************************************/
/*
 * Invokes you callback with each element-child.
 * Return false from callback to abort, true to continue.
 * Returns false if aborted by callback, true if all was processed.
 */

namespace SteamBot
{
    namespace HTML
    {
        bool iterateChildElements(const HTMLParser::Tree::Element&, std::function <bool(size_t, HTMLParser::Tree::Element&)>);
    }
}

/************************************************************************/
/*
 * Check whether this is a node with only whitespace
 */

namespace SteamBot
{
    namespace HTML
    {
        bool isWhitespace(const HTMLParser::Tree::Node&);
    }
}

/************************************************************************/
/*
 * Note sure why I put this in HTML...
 */

namespace SteamBot
{
    namespace HTML
    {
        void trimWhitespace(std::string_view&);
    }
}

/************************************************************************/
/*
 * Check for a specific attribute value
 */

namespace SteamBot
{
    namespace HTML
    {
        bool checkAttribute(const HTMLParser::Tree::Element&, std::string_view, std::string_view);
    }
}

/************************************************************************/
/*
 * Returns a numeric attribute
 */

#include <iostream>

namespace SteamBot
{
    namespace HTML
    {
        template <typename T> std::optional<T> getAttribute(const HTMLParser::Tree::Element& element, std::string_view name)
        {
            std::optional<T> result;

            if (auto string=element.getAttribute(name))
            {
                T value{};
                if (SteamBot::parseNumber(*string, value))
                {
                    result=value;
                }
            }

            return result;
        }
    }
}
