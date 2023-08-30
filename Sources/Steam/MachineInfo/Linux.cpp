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

#ifdef __linux__

/************************************************************************/

#include "Steam/MachineInfo.hpp"

#include <array>
#include <vector>
#include <map>

#include <systemd/sd-id128.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>

#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid_io.hpp>

/************************************************************************/
/*
 * The application-id was geneerated by
 * $ systemd-id128 -p new
 */

static const auto applicationId=SD_ID128_MAKE(f0,ea,ed,d0,71,48,4e,7e,92,54,1e,b4,4e,b6,ed,b3);

/************************************************************************/

const boost::uuids::uuid& Steam::MachineInfo::Provider::getMachineGuid()
{
	class MachineGuid : public boost::uuids::uuid
	{
	public:
		MachineGuid()
		{
			sd_id128_t machineId;
			if (sd_id128_get_machine_app_specific(applicationId, &machineId)!=0)
			{
				throw std::runtime_error("unable to get a machine-id");
			}
			assert(sizeof(data)==16);
			assert(sizeof(machineId.bytes)==16);
			memcpy(data, machineId.bytes, 16);

			BOOST_LOG_TRIVIAL(debug) << "machine-id is " << boost::uuids::to_string(*this);
		}
	};

	static const MachineGuid uuid;
	return uuid;
}

/************************************************************************/
/*
 * Note: this entire thing is stupid, because machines don't have
 * MAC-addresses (or rather, they usually have more than one). So,
 * it's always difficult to decide which one to use for stuff like
 * this...
 *
 * For now, I do this:
 *  - only consider non-loopback interfaces that are up, and have
 *    at least an AF_PACKET (with something that looks like an
 *    ethernet address) and an AF_INET address.
 *  - from these, pick one that has the most addresses in total
 *  - pick an AF_PACKET address from it
 */

class MachineAddress
{
private:
	struct ifaddrs* ifaddrs=nullptr;
	std::map<const std::string, std::vector<const struct ifaddrs*> > interfaces;

private:
	/* populate the 'interfaces' with the ifaddrs that we like, indexed by interface name */
	void getInterfaces()
	{
		for (const struct ifaddrs* ifa=ifaddrs; ifa!=nullptr; ifa=ifa->ifa_next)
		{
			if (ifa->ifa_addr!=nullptr &&
				ifa->ifa_name!=nullptr &&
				(ifa->ifa_flags & IFF_UP) &&
				!(ifa->ifa_flags & IFF_LOOPBACK))
			{
				if (ifa->ifa_addr->sa_family!=AF_PACKET || static_cast<const sockaddr_ll*>(static_cast<const void*>(ifa->ifa_addr))->sll_halen==6)
				{
					interfaces[ifa->ifa_name].push_back(ifa);
				}
			}
		}
	}

private:
	/*
	 * From a list of ifaddrs, return an AF_PACKET item IF we have at
	 * least one AF_PACKET and AF_INET. Else, return nullptr.
	 */
	const struct ifaddrs* getPacketItem(const std::vector<const struct ifaddrs*>& addrs) const
	{
		bool hasInet=false;
		const struct ifaddrs* packet=nullptr;

		for (const struct ifaddrs* ifa : addrs)
		{
			switch(ifa->ifa_addr->sa_family)
			{
			case AF_INET:
				hasInet=true;
				break;

			case AF_PACKET:
				if (packet==nullptr) packet=ifa;
				break;

            default:
                break;
			}
		}

		return hasInet ? packet : nullptr;
	}

private:
	/*
	 * I'm looking for an interface with both AF_PACKET and AF_INET
	 * addresses.
	 * This returns an AF_PACKET for an interface with the most
	 * addresses in total (which I hope is the most important one?)
	 */
	const struct ifaddrs* getBestInterface() const
	{
		class
		{
		private:
			size_t counter=0;
			const struct ifaddrs* ifaddr=nullptr;

		public:
			void updateMaybe(size_t newCounter, const struct ifaddrs* newIfaddr)
			{
				if (newCounter>counter)
				{
					counter=newCounter;
					ifaddr=newIfaddr;
				}
			}

			auto get() const
			{
				return ifaddr;
			}
		} best;

		for (const auto& item : interfaces)
		{
			auto packet=getPacketItem(item.second);
			if (packet!=nullptr)
			{
				best.updateMaybe(item.second.size(), packet);
			}
		}

		return best.get();
	}

public:
	MachineAddress()
	{
		if (getifaddrs(&ifaddrs)!=0)
		{
			throw std::runtime_error("unable to getifaddrs()");
		}
		getInterfaces();
	}

	Steam::MachineInfo::EthernetAddress get() const
	{
		auto packet=getBestInterface();
		if (packet==nullptr)
		{
			throw std::runtime_error("could not find a network interface to supply a MAC address");
		}
		assert(packet->ifa_addr->sa_family==AF_PACKET);
		const auto sockaddr=static_cast<const sockaddr_ll*>(static_cast<const void*>(packet->ifa_addr));
		Steam::MachineInfo::EthernetAddress result(std::span<const uint8_t, 6>(sockaddr->sll_addr, sockaddr->sll_halen));
		BOOST_LOG_TRIVIAL(debug) << "machine MAC address " << result.getString() << " from interface \"" << packet->ifa_name << "\"";
		return result;
	}

	~MachineAddress()
	{
		freeifaddrs(ifaddrs);
	}
};


/************************************************************************/

const Steam::MachineInfo::EthernetAddress& Steam::MachineInfo::Provider::getMachineAddress()
{
	static const EthernetAddress address=MachineAddress().get();
	return address;
}

/************************************************************************/
/*
 * We could try doing something properly
 *    https://www.linuxquestions.org/questions/programming-9/api-to-get-partition-name-based-on-file-path-in-c-on-llinux-os-892461/
 * But I'm too lazy now...
 */

const std::string& Steam::MachineInfo::Provider::getDiskId()
{
	static const std::string diskId("SteamBot-DiskId");
	return diskId;
}

/************************************************************************/

#endif /* __linux__ */
