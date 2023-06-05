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
