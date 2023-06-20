#include "Modules/GetBadgeData.hpp"

/************************************************************************/

typedef SteamBot::Modules::GetPageData::Whiteboard::BadgeData BadgeData;

/************************************************************************/

BadgeData::BadgeInfo::~BadgeInfo() =default;

/************************************************************************/

boost::json::value BadgeData::BadgeInfo::toJson() const
{
    boost::json::object json;
    if (level) json["Level"]=*level;
    if (cardsReceived) json["Received"]=*cardsReceived;
    if (cardsRemaining) json["Remaining"]=*cardsRemaining;
    return json;
}

/************************************************************************/

boost::json::value BadgeData::toJson() const
{
    boost::json::array json;
    for (const auto& badge : badges)
    {
        boost::json::object item;
        item["AppID"]=static_cast<std::underlying_type_t<SteamBot::AppID>>(badge.first);
        item["data"]=badge.second.toJson();
        json.push_back(std::move(item));
    }
    return json;
}
