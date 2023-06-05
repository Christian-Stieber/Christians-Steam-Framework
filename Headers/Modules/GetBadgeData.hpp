#pragma once

#include "MiscIDs.hpp"

#include <unordered_map>
#include <optional>
#include <string>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace GetPageData
        {
            namespace Whiteboard
            {
                class BadgePageData
                {
                public:
                    class BadgeInfo
                    {
                    public:
                        std::optional<unsigned int> level;
                        std::optional<unsigned int> cardsReceived;
                        std::optional<unsigned int> cardsRemaining;
                    };

                private:
                    std::unordered_map<AppID, BadgeInfo> badges;

                public:
                    BadgePageData(std::string_view);
                    ~BadgePageData();

                public:
                    const decltype(badges)& getBadges() const
                    {
                        return badges;
                    }
                };
            }
        }
    }
}
