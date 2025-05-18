#include "Helpers/HTML.hpp"

#include "HTMLParser/Tree.hpp"

#include <cassert>

/************************************************************************/
/*
 * Pass in a lower-case class name
 *
 * Note: I don't expect me to pass anything but a literal for
 * className anyway, so the fact that we can have a nullptr matcher is
 * just too useful. Using std::string_view just looks clumsy.
 */

bool SteamBot::HTML::checkClass(std::string_view classString, const char* className)
{
    const char* matcher=nullptr;
    bool mismatch=false;

    for (char c : classString)
    {
        if (c==' ')
        {
            if (matcher!=nullptr)
            {
                if (!mismatch && *matcher=='\0')
                {
                    return true;
                }
                matcher=nullptr;
                mismatch=false;
            }
        }
        else
        {
            if (c>='A' && c<='Z')
            {
                c|=0x20;
            }
            if (matcher==nullptr)
            {
                matcher=className;
            }
            if (!mismatch)
            {
                if (*matcher==c)
                {
                    matcher++;
                }
                else
                {
                    mismatch=true;
                }
            }
        }
    }
    return (!mismatch && matcher!=nullptr && *matcher=='\0');
}

/************************************************************************/

bool SteamBot::HTML::checkClass(const HTMLParser::Tree::Element& element, const char* className)
{
    if (auto classAttribute=element.getAttribute("class"))
    {
        return checkClass(*classAttribute, className);
    }
    return false;
}

/************************************************************************/

std::string SteamBot::HTML::getCleanText(const HTMLParser::Tree::Element& element)
{
    struct Collector
    {
        std::string result;

        void collect(const HTMLParser::Tree::Element& element)
        {
            for (const auto& node : element.children)
            {
                if (auto textChild=dynamic_cast<const HTMLParser::Tree::Text*>(node.get()))
                {
                    for (char c : textChild->text)
                    {
                        if (c==0x09 || c==0x0a || c==0x0d)
                        {
                            c=' ';
                        }
                        if (c==' ')
                        {
                            if (!result.empty() && result.back()!=' ')
                            {
                                result.push_back(c);
                            }
                        }
                        else
                        {
                            result.push_back(c);
                        }
                    }
                }
                else if (auto elementChild=dynamic_cast<const HTMLParser::Tree::Element*>(node.get()))
                {
                    collect(*elementChild);
                }
                else
                {
                    assert(false);
                }
            }
        }
    };

    Collector collector;
    collector.collect(element);
    if (!collector.result.empty() && collector.result.back()==' ')
    {
        collector.result.pop_back();
    }
    return collector.result;
}

/************************************************************************/

void SteamBot::HTML::trimWhitespace(std::string_view& string)
{
    while (!string.empty())
    {
        char c=string.front();
        if (c!=' ' && c!=0x09 && c!=0x0a && c!=0x0d)
        {
            break;
        }
        string.remove_prefix(1);
    }

    while (!string.empty())
    {
        char c=string.back();
        if (c!=' ' && c!=0x09 && c!=0x0a && c!=0x0d)
        {
            break;
        }
        string.remove_suffix(1);
    }
}

/************************************************************************/

bool SteamBot::HTML::isWhitespace(const HTMLParser::Tree::Node& node)
{
    if (auto text=dynamic_cast<const HTMLParser::Tree::Text*>(&node))
    {
        for (const char c: text->text)
        {
            if (c!=0x09 && c!=0x0a && c!=0x0d) return false;
        }
        return true;
    }
    return false;
}

/************************************************************************/

bool SteamBot::HTML::checkAttribute(const HTMLParser::Tree::Element& element, std::string_view name, std::string_view value)
{
    if (auto string=element.getAttribute(name))
    {
        if (*string==value)
        {
            return true;
        }
    }
    return false;
}

/************************************************************************/

ssize_t SteamBot::HTML::iterateChildElements(const HTMLParser::Tree::Element& element, std::function <bool(size_t, HTMLParser::Tree::Element&)> callback)
{
    size_t count=0;
    for (const auto& node : element.children)
    {
        auto child=dynamic_cast<HTMLParser::Tree::Element*>(node.get());
        if (child!=nullptr)
        {
            if (!callback(count, *child))
            {
                return -1;
            }
            count++;
        }
    }
    return static_cast<ssize_t>(count);
}
