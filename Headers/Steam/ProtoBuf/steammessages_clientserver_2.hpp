#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_clientserver_2.pb.h"

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientUpdateMachineAuth,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientUpdateMachineAuth> CMsgClientUpdateMachineAuthMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientUpdateMachineAuthResponse,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientUpdateMachineAuthResponse> CMsgClientUpdateMachineAuthResponseMessageType;
}
