#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_clientserver_friends.pb.h"

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientFriendsList,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientFriendsList> CMsgClientFriendsListMessageType;
}

/************************************************************************/

namespace Steam
{
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ClientChangeStatus,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgClientChangeStatus> CMsgClientChangeStatusMessageType;
}
