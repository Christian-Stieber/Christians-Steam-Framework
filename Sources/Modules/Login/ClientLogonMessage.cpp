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

#include "SteamID.hpp"
#include "ResultCode.hpp"
#include "Steam/MsgClientLogon.hpp"
#include "Steam/MachineInfo.hpp"

/************************************************************************/

static bool isEmpty(const boost::optional<std::string>& string)
{
	if (!string)
	{
		return true;
	}
	return (*string).empty();
}

/************************************************************************/

template <typename T> auto toInteger(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}

/************************************************************************/

void SteamBot::Modules::Login::Internal::fillClientLogonMessage(Steam::CMsgClientLogonMessageType& logon,
                                                                const SteamBot::Modules::Login::Internal::LoginDetails& details,
                                                                const SteamBot::Connection::Endpoint localEndpoint)
{
	assert(!isEmpty(details.username));
	assert(!(isEmpty(details.password) && isEmpty(details.loginKey)));

	assert(!(!isEmpty(details.loginKey) && !details.shouldRememberPassword));

	{
		SteamBot::SteamID steamID;
		steamID.setAccountId(details.accountID);
		steamID.setAccountInstance(toInteger(details.accountInstance));
		steamID.setUniverseType(SteamBot::Client::getClient().universe.type);
		steamID.setAccountType(Steam::AccountType::Individual);
		logon.header.proto.set_steamid(static_cast<uint64_t>(steamID));
	}

	// TODO: Support IPv6 login ids?
	{
		uint32_t ipv4=0;
		if (details.loginId)
		{
			ipv4=*details.loginId;
		}
		else
		{
			static const uint32_t obfuscationMask=0xBAADF00D;
			// ToDo: why are we obfuscating like that? It serves no purpose...
			if (localEndpoint.address.is_v4())
			{
				const uint32_t address=localEndpoint.address.to_v4().to_uint();
				ipv4=address^obfuscationMask;
			}
		}
		if (ipv4!=0)
		{
			logon.content.mutable_obfuscated_private_ip()->set_v4(ipv4);
			logon.content.set_deprecated_obfustucated_private_ip(ipv4);
		}
	}

	if (details.cellId)
	{
		logon.content.set_cell_id(*details.cellId);
	}
	else
	{
		// logon.Body.cell_id = details.CellID ?? Client.Configuration.CellID;
	}

	logon.header.proto.set_client_sessionid(0);

	if (details.username) logon.content.set_account_name(*details.username);
	if (details.password) logon.content.set_password(*details.password);
	logon.content.set_should_remember_password(details.shouldRememberPassword);

	logon.content.set_protocol_version(Steam::MsgClientLogon::CurrentProtocol);
	logon.content.set_client_os_type(toInteger(details.clientOSType));
	logon.content.set_client_language(details.clientLanguage);

	logon.content.set_steam2_ticket_request(details.requestSteam2Ticket);

	// SteamKit2 -- we're now using the latest steamclient package version, this is required to get a proper sentry file for steam guard
	logon.content.set_client_package_version(1771);		// SteamKit2 -- todo: determine if this is still required
	logon.content.set_supports_rate_limit_response(true);
	{
		const auto& machineId=Steam::MachineInfo::MachineID::getSerialized();
		logon.content.set_machine_id(machineId.data(), machineId.size());
	}
	logon.content.set_machine_name(Steam::MachineInfo::Provider::getMachineName());

	// SteamKit2 -- steam guard 
	if (details.steamGuardAuthCode) logon.content.set_auth_code(*details.steamGuardAuthCode);
	if (details.app2FactorCode) logon.content.set_two_factor_code(*details.app2FactorCode);

	if (details.loginKey) logon.content.set_login_key(*details.loginKey);

	if (details.sentryFileHash.empty())
	{
		logon.content.set_eresult_sentryfile(toInteger(SteamBot::ResultCode::FileNotFound));
	}
	else
	{
		logon.content.set_sha_sentryfile(details.sentryFileHash);
		logon.content.set_eresult_sentryfile(toInteger(SteamBot::ResultCode::OK));
	}
}

