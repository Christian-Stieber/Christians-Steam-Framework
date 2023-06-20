#include "Modules/GetBadgeData.hpp"
#include "Modules/Login.hpp"
#include "Client/Client.hpp"
#include "Helpers/HTML.hpp"

#include "HTMLParser/Parser.hpp"

#include <boost/url/parse.hpp>
#include <boost/log/trivial.hpp>
#include <charconv>

/************************************************************************/

typedef SteamBot::Modules::GetPageData::Whiteboard::BadgePageData BadgePageData;

/************************************************************************/

namespace
{
    class BadgePageParser : public HTMLParser::Parser
    {
    private:
        BadgePageData& data;

        // The root elements for the badge-boxes on the page
        // Basically, we just walk up from lower elements until we find a match
        std::unordered_map<const HTMLParser::Tree::Element*, SteamBot::AppID> badge_row;

    private:
        bool stripCardPrefix(std::string_view&);

    public:
        BadgePageParser(std::string_view html, BadgePageData& data_)
            : Parser(html), data(data_)
        {
        }

        virtual ~BadgePageParser() =default;

    private:
        static bool extractNumber(const HTMLParser::Tree::Element&, unsigned int&);
        BadgePageData::BadgeInfo* findBadgeInfo(const HTMLParser::Tree::Element&) const;

    private:
        bool handleStart_badge_row_overlay(const HTMLParser::Tree::Element&);
        bool handleEnd_progress_info_bold(const HTMLParser::Tree::Element&);

    private:
        virtual void startElement(const HTMLParser::Tree::Element& element) override
        {
            handleStart_badge_row_overlay(element);
        }

        virtual void endElement(const HTMLParser::Tree::Element& element) override
        {
            handleEnd_progress_info_bold(element);
        }

    };
}

/************************************************************************/
/*
 * Card links look like this:
 *   https://steamcommunity.com/profiles/<SteamID>/gamecards/<AppID>/
 *   https://steamcommunity.com/id/<CustomName>/gamecards/<AppID>/
 *
 * This removes the text up to the "<AppID>/"
 */

bool BadgePageParser::stripCardPrefix(std::string_view& url)
{
    struct Helper
    {
        static bool removePrefix(std::string_view& string, const std::string_view prefix)
        {
            if (string.starts_with(prefix))
            {
                string.remove_prefix(prefix.size());
                return true;
            }
            return false;
        }

        static bool removeUntil(std::string_view& string, char c)
        {
            while (!string.empty())
            {
                const char x=string.front();
                string.remove_prefix(1);
                if (c==x)
                {
                    return true;
                }
            }
            return false;
        }
    };

    return
        Helper::removePrefix(url, "https://steamcommunity.com/") &&
        (Helper::removePrefix(url, "profiles/") || Helper::removePrefix(url, "id/")) &&
        Helper::removeUntil(url, '/') &&
        Helper::removePrefix(url, "gamecards/");
}

/************************************************************************/

bool BadgePageParser::handleStart_badge_row_overlay(const HTMLParser::Tree::Element& element)
{
    // <a class="badge_row_overlay" href="https://steamcommunity.com/profiles/<SteamID>/gamecards/<AppID>/"></a>

    if (element.name=="a" && SteamBot::HTML::checkClass(element, "badge_row_overlay"))
    {
        if (auto href=element.getAttribute("href"))
        {
            std::string_view string(*href);
            if (stripCardPrefix(string))
            {
                std::underlying_type_t<SteamBot::AppID> value;
                const char* last=string.data()+string.size();
                assert(*last=='\0');
                auto result=std::from_chars(string.data(), last, value);
                if (result.ec==std::errc())
                {
                    if (*result.ptr=='\0' || *result.ptr=='/')
                    {
                        auto& parent=*(element.parent);
                        if (parent.name=="div" && SteamBot::HTML::checkClass(parent, "badge_row"))
                        {
                            data.badges.try_emplace(static_cast<SteamBot::AppID>(value));

                            auto result=badge_row.emplace(&parent, static_cast<SteamBot::AppID>(value));
                            assert(result.second);
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/
/*
 * ToDo: check for multiple numbers?
 * ToDo: check for numbers inside of words?
 */

bool BadgePageParser::extractNumber(const HTMLParser::Tree::Element& element, unsigned int& value)
{
    auto text=SteamBot::HTML::getCleanText(element);
    for (const char* s=text.data(); *s!='\0'; s++)
    {
        if (*s>='0' && *s<='9')
        {
            value=0;
            do
            {
                value=10*value+(*s-'0');
                s++;
            }
            while (*s>='0' && *s<='9');
            return true;
        }
    }
    return false;
}

/************************************************************************/
/*
 * Walk up until we find a badge_row, then return the BadgeInfo
 * associated with it. nullptr if not found.
 */

BadgePageData::BadgeInfo* BadgePageParser::findBadgeInfo(const HTMLParser::Tree::Element& element) const
{
    for (const HTMLParser::Tree::Element* parent=element.parent; parent!=nullptr; parent=parent->parent)
    {
        auto nodeIterator=badge_row.find(parent);
        if (nodeIterator!=badge_row.end())
        {
            SteamBot::AppID appId=nodeIterator->second;
            auto badgeIterator=data.badges.find(appId);
            if (badgeIterator!=data.badges.end())
            {
                return &(badgeIterator->second);
            }
            break;
        }
    }
    return nullptr;
}

/************************************************************************/

bool BadgePageParser::handleEnd_progress_info_bold(const HTMLParser::Tree::Element& element)
{
    // <span class="progress_info_bold">...</span>

    if (element.name=="span" && SteamBot::HTML::checkClass(element, "progress_info_bold"))
    {
        unsigned int cardsRemaining;
        if (extractNumber(element, cardsRemaining))
        {
            if (auto badgeInfo=findBadgeInfo(element))
            {
                badgeInfo->cardsRemaining=badgeInfo->cardsRemaining.value_or(0)+cardsRemaining;
            }
        }
        return true;
    }
    return false;
}

/************************************************************************/

BadgePageData::BadgePageData(std::string_view html)
{
    BadgePageParser(html, *this).parse();
}

/************************************************************************/

BadgePageData::~BadgePageData() =default;
