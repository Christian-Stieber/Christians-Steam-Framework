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
                if (auto child=dynamic_cast<const HTMLParser::Tree::Text*>(node.get()))
                {
                    for (char c : child->text)
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
                else if (auto child=dynamic_cast<const HTMLParser::Tree::Element*>(node.get()))
                {
                    collect(*child);
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
