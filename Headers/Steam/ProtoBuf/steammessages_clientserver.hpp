#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_clientserver.pb.h"

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientLicenseList,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientLicenseList> CMsgClientLicenseListMessageType;
}

/************************************************************************/

namespace Steam
{
    // Note: ArchiSteamFarm uses ClientGamesPlayedWithDataBlob, but it
    // seems to work with ClientGamesPlayed just fine

	typedef SteamBot::Connection::Message::Message<
#if 1
        SteamBot::Connection::Message::Type::ClientGamesPlayed,
#else
        SteamBot::Connection::Message::Type::ClientGamesPlayedWithDataBlob,
#endif
        SteamBot::Connection::Message::Header::ProtoBuf,
        CMsgClientGamesPlayed> CMsgClientGamesPlayedMessageType;
}
