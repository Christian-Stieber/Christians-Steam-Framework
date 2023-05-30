#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_clientserver_login.pb.h"

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientLogon,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientLogon> CMsgClientLogonMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientLogOnResponse,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientLogonResponse> CMsgClientLogonResponseMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientAccountInfo,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientAccountInfo> CMsgClientAccountInfoMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientHeartBeat,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientHeartBeat> CMsgClientHeartBeatMessageType;
}
