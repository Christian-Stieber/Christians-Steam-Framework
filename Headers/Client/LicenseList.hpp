#pragma once

#include "Steam/ProtoBuf/steammessages_clientserver.hpp"

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		class LicenseList
		{
		public:
			void handleMessage(boost::asio::yield_context, std::shared_ptr<CMsgClientLicenseListMessageType>);

		public:
			LicenseList(Client&);
			~LicenseList();
		};
	}
}
