#pragma once

#include "Printable.hpp"
#include "MiscIDs.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace PlayGames
        {
            namespace Messageboard
            {
                class PlayGame : public Printable
                {
                public:
                    AppID appId=AppID::None;

                public:
                    PlayGame();
                    virtual ~PlayGame();
                    virtual boost::json::value toJson() const override;

                public:
                    static void play(AppID);
                };
            }
        }
    }
}
