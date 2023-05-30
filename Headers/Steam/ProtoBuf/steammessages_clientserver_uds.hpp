#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_clientserver_uds.pb.h"

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientGetClientAppList,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientGetClientAppList> CMsgClientGetClientAppListMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientGetClientAppListResponse,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientGetClientAppListResponse> CMsgClientGetClientAppListResponseMessageType;
}
