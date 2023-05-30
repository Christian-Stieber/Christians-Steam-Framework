#pragma once

#include "MiscIDs.hpp"
#include "Printable.hpp"

#include <string>
#include <unordered_map>
#include <memory>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace OwnedGames
        {
            namespace Whiteboard
            {
                class OwnedGames : public Printable
                {
                public:
                    class GameInfo : public Printable
                    {
                    public:
                        GameInfo();
                        virtual ~GameInfo();
                        virtual boost::json::value toJson() const override;

                    public:
                        AppID appId;
                        std::string name;
                    };

                public:
                    std::unordered_map<AppID, std::shared_ptr<const GameInfo>> games;

                public:
                    OwnedGames();
                    virtual ~OwnedGames();
                    virtual boost::json::value toJson() const override;
                };
            }
        }
    }
}
