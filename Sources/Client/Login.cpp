#include "Client/Client.hpp"
#include "Client/Login.hpp"
#include "Client/Result.hpp"
#include "Client/MsgClientLogon.hpp"
#include "EnumToInteger.hpp"
#include "SteamID.hpp"
#include "Steam/MachineInfo.hpp"
#include "Config.hpp"
#include "Client/AccountFlags.hpp"

#include <boost/log/trivial.hpp>

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

void fillLogonMessage(Steam::CMsgClientLogonMessageType& logon, const Steam::Client::Login::LoginDetails& details,
					  const Steam::Universe& universe, const Steam::Client::Connection::Endpoint& localEndpoint)
{
	assert(!isEmpty(details.username));
	assert(!(isEmpty(details.password) && isEmpty(details.loginKey)));

	assert(!(!isEmpty(details.loginKey) && !details.shouldRememberPassword));

	{
		Steam::SteamID steamID;
		steamID.setAccountId(details.accountID);
		steamID.setAccountInstance(Steam::toInteger(details.accountInstance));
		steamID.setUniverseType(universe.type);
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
			if (localEndpoint.address().is_v4())
			{
				const uint32_t address=localEndpoint.address().to_v4().to_uint();
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
	logon.content.set_client_os_type(Steam::toInteger(details.clientOSType));
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
		logon.content.set_eresult_sentryfile(Steam::toInteger(Steam::Client::ResultCode::FileNotFound));
	}
	else
	{
		logon.content.set_sha_sentryfile(details.sentryFileHash);
		logon.content.set_eresult_sentryfile(Steam::toInteger(Steam::Client::ResultCode::OK));
	}
}

/************************************************************************/

void Steam::Client::Client::handleMessage(boost::asio::yield_context, std::shared_ptr<CMsgClientAccountInfoMessageType> message)
{
	if (message->content.has_account_flags())
	{
		auto flags=static_cast<Steam::Client::AccountFlags>(message->content.account_flags());
		BOOST_LOG_TRIVIAL(debug) << "account flags: " << magic_enum::enum_flags_name(flags);
	}
}

/************************************************************************/

void Steam::Client::Client::handleMessage(boost::asio::yield_context yield, std::shared_ptr<CMsgClientLogonResponseMessageType> message)
{
	if (message->content.has_eresult())
	{
		auto loginResult=static_cast<Steam::Client::ResultCode>(static_cast<std::underlying_type_t<Steam::Client::ResultCode>>(message->content.eresult()));
		BOOST_LOG_TRIVIAL(debug) << "result code: " << SteamBot::enumToStringAlways(loginResult);
		switch(loginResult)
		{
		case Steam::Client::ResultCode::AccountLogonDenied:
		case Steam::Client::ResultCode::InvalidLoginAuthCode:
			restart(RestartReason::SteamGuard);
			break;

		case Steam::Client::ResultCode::OK:
			// SteamKit CMClient.cs -> HandleLogOnResponse
			if (message->header.proto.has_steamid()) steamId=message->header.proto.steamid();
			if (message->header.proto.has_client_sessionid()) sessionId=message->header.proto.client_sessionid();
			if (message->content.has_cell_id()) cellId=message->content.cell_id();
#if 0
			PublicIP = logonResp.Body.public_ip.GetIPAddress();
			IPCountryCode = logonResp.Body.ip_country_code;
#endif

			// ToDo: heartbeat?
			//int hbDelay = logonResp.Body.out_of_game_heartbeat_seconds;
			// restart heartbeat
			// heartBeatFunc.Stop();
			// heartBeatFunc.Delay = TimeSpan.FromSeconds( hbDelay );
			// heartBeatFunc.Start();

			loginWait.wakeup();
			break;

		default:
			break;
		}
	}
}

/************************************************************************/

void Steam::Client::Client::logon(boost::asio::yield_context yield)
{
	assert(connection);

	dispatcher->setHandler<CMsgClientLogonResponseMessageType>(this);
	dispatcher->setHandler<CMsgClientAccountInfoMessageType>(this);
	dispatcher->setHandler<CMsgClientUpdateMachineAuthMessageType>(this);

	Steam::Client::Login::LoginDetails details;
	{
		const auto& account=SteamBot::Config::SteamAccount::get();
		details.username=account.user;
		details.password=account.password;

		addSteamGuardData(details, yield);
	}

	Steam::CMsgClientLogonMessageType message;
	{
		Steam::Client::Connection::Endpoint localEndpoint;
		connection->getLocalAddress(localEndpoint);
		fillLogonMessage(message, details, universe, localEndpoint);
	}

	send(message, yield);

	loginWait.wait(yield);
}
