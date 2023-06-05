#pragma once

#include "MiscIDs.hpp"
#include "Printable.hpp"

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
                class BadgePageData : public Printable
                {
                public:
                    class BadgeInfo : public Printable
                    {
                    public:
                        std::optional<unsigned int> level;
                        std::optional<unsigned int> cardsReceived;
                        std::optional<unsigned int> cardsRemaining;

                    public:
                        virtual ~BadgeInfo();

                        virtual boost::json::value toJson() const override;
                    };

                public:
                    std::unordered_map<AppID, BadgeInfo> badges;

                public:
                    BadgePageData(std::string_view);
                    virtual ~BadgePageData();

                    virtual boost::json::value toJson() const override;
                };
            }
        }
    }
}
