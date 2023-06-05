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

        /* https://steamcommunity.com/profiles/<SteamID>/gamecards/ */
        std::string cardsPrefix;

    private:
        void setCardsPrefix()
        {
            auto* sessionInfo=SteamBot::Client::getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SessionInfo>();
            assert(sessionInfo!=nullptr);
            assert(sessionInfo->steamId);

            cardsPrefix="https://steamcommunity.com/profiles/";
            cardsPrefix.append(std::to_string(sessionInfo->steamId->getValue()));
            cardsPrefix.append("/gamecards/");
        }

    public:
        BadgePageParser(std::string_view html, BadgePageData& data_)
            : Parser(html), data(data_)
        {
            setCardsPrefix();
        }

        virtual ~BadgePageParser() =default;

    private:
        bool handleStart_badge_row_overlay(const HTMLParser::Tree::Element&);

    private:
        virtual void startElement(const HTMLParser::Tree::Element& element) override
        {
            if (handleStart_badge_row_overlay(element))
            {
            }
        }
    };
}

/************************************************************************/

bool BadgePageParser::handleStart_badge_row_overlay(const HTMLParser::Tree::Element& element)
{
    // <a class="badge_row_overlay" href="https://steamcommunity.com/profiles/<SteamID>/gamecards/<AppID>/"></a>

    if (element.name=="a")
    {
        if (SteamBot::HTML::checkClass(element, "badge_row_overlay"))
        {
            if (auto href=element.getAttribute("href"))
            {
                if (href->starts_with(cardsPrefix))
                {
                    std::underlying_type_t<SteamBot::AppID> value;
                    const char* last=href->data()+href->size();
                    assert(*last=='\0');
                    auto result=std::from_chars(href->data()+cardsPrefix.size(), last, value);
                    if (result.ec==std::errc())
                    {
                        if (*result.ptr=='\0' || *result.ptr=='/')
                        {
                            BOOST_LOG_TRIVIAL(debug) << "Found app ID: " << value;
                        }
                    }
                }
            }
            return true;
        }
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
