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

#include "Steam/MachineInfo.hpp"
#include "OpenSSL/SHA1.hpp"

#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/

static char hexDigit(uint8_t digit)
{
	if (digit<10) return '0'+digit;
	if (digit<16) return 'a'-10+digit;
	assert(false);
	return '?';
}

/************************************************************************/

static void appendByte(std::string& string, uint8_t byte)
{
	string+=hexDigit(byte>>4);
	string+=hexDigit(byte&0x0f);
}

/************************************************************************/

const std::string& Steam::MachineInfo::EthernetAddress::getString() const
{
	if (string.empty())
	{
		string.reserve(address.size()*3-1);
		for (uint8_t byte : address)
		{
			if (!string.empty()) string+=':';
			appendByte(string, byte);
		}
	}

	return string;
}

/************************************************************************/

static std::string getHexString(std::span<const std::byte> bytes)
{
	std::string result;
	std::array<std::byte, 20> hash;
	SteamBot::OpenSSL::calculateSHA1(bytes, std::span<std::byte, 20>(hash.data(), hash.size()));
	result.reserve(hash.size()*2);
	for (const auto byte : hash)
	{
		appendByte(result, static_cast<uint8_t>(byte));
	}
	return result;
}

/************************************************************************/

static std::string getHexString(const boost::uuids::uuid& uuid)
{
	return getHexString(std::as_bytes(std::span<const uint8_t, 16>(uuid.data, uuid.size())));
}

/************************************************************************/

static std::string getHexString(const Steam::MachineInfo::EthernetAddress& address)
{
	const auto& bytes=address.getBytes();
	return getHexString(std::as_bytes(std::span<const uint8_t, 6>(bytes.data(), bytes.size())));
}

/************************************************************************/

static std::string getHexString(const std::string& string)
{
	return getHexString(std::as_bytes(std::span<const char>(string.data(), string.size())));
}

/************************************************************************/

Steam::MachineInfo::MachineID::MachineID()
{
	tree.setValue<std::string>("BB3", getHexString(Steam::MachineInfo::Provider::getMachineGuid()));
	tree.setValue<std::string>("FF2", getHexString(Steam::MachineInfo::Provider::getMachineAddress()));
	tree.setValue<std::string>("3B3", getHexString(Steam::MachineInfo::Provider::getDiskId()));
}

/************************************************************************/

const Steam::MachineInfo::MachineID& Steam::MachineInfo::MachineID::get()
{
	static const MachineID machineId;
	return machineId;
}

/************************************************************************/

const std::vector<std::byte>& Steam::MachineInfo::MachineID::getSerialized()
{
	static const std::string name("MessageObject");
	static const auto serialized=get().tree.serializeToBinary(name);
	return serialized;
}

/************************************************************************/

const std::string& Steam::MachineInfo::Provider::getMachineName()
{
	static const std::string name=[](){
		boost::system::error_code error;
		std::string result=boost::asio::ip::host_name(error);
		if (error)
		{
			result="unknown";
		}
        result+=" (CSF)";
		BOOST_LOG_TRIVIAL(debug) << "machine name is \"" << result << "\"";
		return result;
	}();
	return name;
}
