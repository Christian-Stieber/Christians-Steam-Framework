/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "Modules/Connection.hpp"
#include "Client/Module.hpp"

#include "Steam/ProtoBuf/steammessages_base.hpp"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

/************************************************************************/

namespace
{
    class MultiPacketModule : public SteamBot::Client::ModuleBase
    {
    public:
        MultiPacketModule() =default;
        virtual ~MultiPacketModule() =default;

        virtual void run() override;

    private:
        void handle(std::shared_ptr<const Steam::CMsgMultiMessageType>);
    };

    MultiPacketModule::Init<MultiPacketModule> init;
}

/************************************************************************/

static std::span<const std::byte> makePayload(const std::string& string)
{
	return std::span<const std::byte>(static_cast<const std::byte*>(static_cast<const void*>(string.data())), string.size());
}

/************************************************************************/

void MultiPacketModule::handle(std::shared_ptr<const Steam::CMsgMultiMessageType> message)
{
    if (message->content.has_message_body())
	{
		std::string unzipped;	// we need it as a buffer inside the scope

		auto payload=makePayload(message->content.message_body());

		if (message->content.has_size_unzipped())
		{
			auto size=message->content.size_unzipped();
			if (size>0)
			{
				{
					boost::iostreams::filtering_ostream stream;
					stream.push(boost::iostreams::gzip_decompressor{});
					stream.push(boost::iostreams::back_inserter(unzipped));
                    const char* bytes=static_cast<const char*>(static_cast<const void*>(payload.data()));
					boost::iostreams::write(stream, bytes, payload.size());
					stream.strict_sync();
					payload=makePayload(unzipped);
				}
				assert(payload.size()==size);
			}
		}

		SteamBot::Connection::Deserializer deserializer(payload);
		try
		{
			while (deserializer.data.size()>0)
			{
				auto size=deserializer.get<uint32_t>();
				if (size>0)
				{
					auto subBytes=deserializer.getBytes(size);
                    SteamBot::Modules::Connection::handlePacket(subBytes);
				}
			}
		}
		catch(const decltype(deserializer)::NotEnoughDataException&)
		{
			BOOST_LOG_TRIVIAL(info) << "CMsgMulti parsing ran out of data?";
		}
    }
}

/************************************************************************/

void MultiPacketModule::run()
{
    getClient().launchFiber("MultiPacketModule::run", [this](){
        auto waiter=SteamBot::Waiter::create();

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgMultiMessageType>> multiMessageWaiter;
        multiMessageWaiter=waiter->createWaiter<decltype(multiMessageWaiter)::element_type>(getClient().messageboard);

        auto cancellation=getClient().cancel.registerObject(*waiter);

        while (true)
        {
            waiter->wait();
            auto message=multiMessageWaiter->fetch();
            if (message)
            {
                handle(message);
            }
        }
    });
}
