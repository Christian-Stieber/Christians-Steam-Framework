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

#include "./Login.hpp"

#include "Client/DataFile.hpp"
#include "OpenSSL/SHA1.hpp"
#include "Base64.hpp"
#include "ResultCode.hpp"
#include "Modules/Connection.hpp"

/************************************************************************/

static const std::string sentryFileKey="SteamGuard.sentryFile";

/************************************************************************/

static std::vector<std::byte> getSentryFile(SteamBot::DataFile& dataFile)
{
	std::vector<std::byte> file;

	const auto string=dataFile.examine([](const boost::property_tree::ptree& tree) {
		return tree.get_optional<std::string>(sentryFileKey);
	});
	if (string)
	{
		file=SteamBot::Base64::decode(*string);
	}

	return file;
}

/************************************************************************/

static void storeSentryFile(SteamBot::DataFile& dataFile, const std::vector<std::byte>& bytes)
{
	const auto string=SteamBot::Base64::encode(bytes);
	dataFile.update([&string](boost::property_tree::ptree& tree) {
		tree.put(sentryFileKey, std::move(string));
	});
}

/************************************************************************/

static std::string getSentryHash(const std::vector<std::byte>& file)
{
	std::string result;
	result.resize(20);
	SteamBot::OpenSSL::calculateSHA1(std::span<const std::byte>(file),
                                     std::span<std::byte, 20>(static_cast<std::byte*>(static_cast<void*>(result.data())), result.size()));
	return result;
}

/************************************************************************/

static std::span<const std::byte> makeBytes(const std::string& string)
{
	return std::span<const std::byte>(static_cast<const std::byte*>(static_cast<const void*>(string.data())), string.size());
}

/************************************************************************/
/*
 * We get this after a successful login with a SteamGuard code. It's
 * basicaly giving us a sentry file that we'll be using for future
 * logins so SteamGuard won't trigger.
 */

void SteamBot::Modules::Login::Internal::handleCmsgClientUpdateMachineAuth(std::shared_ptr<const Steam::CMsgClientUpdateMachineAuthMessageType> message)
{
	if (message->content.has_bytes())
	{
		// Note: we just ignore the "filename" (ArchiSteamFarm does it too)
		auto bytes=makeBytes(message->content.bytes());
		const uint32_t offset=message->content.has_offset() ? message->content.offset() : 0;
		const uint32_t cubtowrite=message->content.has_cubtowrite() ? message->content.cubtowrite() : bytes.size();
		assert(cubtowrite<=bytes.size());

		auto file=getSentryFile(SteamBot::Client::getClient().dataFile);
		{
			if (file.size()<offset)
			{
				file.resize(offset, static_cast<std::byte>(0));
			}
			if (file.size()<offset+cubtowrite)
			{
				file.resize(offset+cubtowrite);
			}
			std::memcpy(static_cast<uint8_t*>(static_cast<void*>(file.data()))+offset, bytes.data(), cubtowrite);
		}
		storeSentryFile(SteamBot::Client::getClient().dataFile, file);

        auto response=std::make_unique<Steam::CMsgClientUpdateMachineAuthResponseMessageType>();
		{
			if (message->header.proto.has_jobid_source()) response->header.proto.set_jobid_target(message->header.proto.jobid_source());

            response->content.set_eresult(static_cast<std::underlying_type_t<ResultCode>>(ResultCode::OK));
            response->content.set_getlasterror(0);

			response->content.set_cubwrote(cubtowrite);
			if (message->content.has_filename()) response->content.set_filename(message->content.filename());
			response->content.set_filesize(file.size());
            response->content.set_offset(offset);
			response->content.set_sha_file(::getSentryHash(file));

			if (message->content.has_otp_type()) response->content.set_otp_type(message->content.otp_type());
			if (message->content.has_otp_identifier()) response->content.set_otp_identifier(message->content.otp_identifier());
		}
        SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(response));
	}
}

/************************************************************************/

std::string SteamBot::Modules::Login::Internal::getSentryHash()
{
    std::string result;
	const auto file=getSentryFile(SteamBot::Client::getClient().dataFile);
	if (file.size()>0)
	{
		result=::getSentryHash(file);
	}
    return result;
}
