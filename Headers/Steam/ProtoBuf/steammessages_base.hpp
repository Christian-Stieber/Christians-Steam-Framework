#pragma once

/************************************************************************/

#include "Connection/Message.hpp"

#include "steamdatabase/protobufs/steam/steammessages_base.pb.h"

/************************************************************************/

namespace Steam
{
    typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::Multi,
                                                   SteamBot::Connection::Message::Header::ProtoBuf,
                                                   CMsgMulti> CMsgMultiMessageType;
}
