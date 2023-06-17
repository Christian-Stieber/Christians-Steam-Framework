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

#pragma once

#include "Steam/KeyValue.hpp"

#include <array>
#include <span>
#include <cstring>

/************************************************************************/

namespace boost
{
	namespace uuids
	{
		struct uuid;
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
