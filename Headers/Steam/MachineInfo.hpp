#pragma once

#include "Steam/KeyValue.hpp"

#include <span>
#include <cstring>

/************************************************************************/

namespace boost
{
	namespace uuids
	{
		class uuid;
	}
}

/************************************************************************/

namespace Steam
{
	namespace MachineInfo
	{
		class EthernetAddress
		{
		private:
			std::array<uint8_t, 6> address;
			mutable std::string string;

		public:
			EthernetAddress(std::span<const uint8_t, 6> bytes)
			{
				memcpy(address.data(), bytes.data(), 6);
			}

		public:
			const decltype(address)& getBytes() const
			{
				return address;
			}

			const std::string& getString() const;
		};
	}
}

/************************************************************************/
/*
 * This is a class to provide information about the current machine.
 *
 * There are vastly different implementaions for the supported
 * operating systems.
 */

namespace Steam
{
	namespace MachineInfo
	{
		class Provider
		{
			Provider() =delete;

		public:
			static const boost::uuids::uuid& getMachineGuid();
			static const EthernetAddress& getMachineAddress();
			static const std::string& getDiskId();
			static const std::string& getMachineName();
		};
	}
}

/************************************************************************/
/*
 * SteamKit2, SteamKit2/Util/HardwareUtils.cs, MachineID
 *
 * We should really only need one instance, right?
 */

namespace Steam
{
	namespace MachineInfo
	{
		class MachineID
		{
		private:
			Steam::KeyValue::Tree tree;

		private:
			MachineID();

		public:
			static const MachineID& get();
			static const std::vector<std::byte>& getSerialized();
		};
	}
}
