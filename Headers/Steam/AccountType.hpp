#pragma once

/************************************************************************/
/*
 * SteamKit2, SteamLanguage.cs, EAccountType
 */

namespace Steam
{
	enum class AccountType {
		Invalid = 0,
		Individual = 1,
		Multiseat = 2,
		GameServer = 3,
		AnonGameServer = 4,
		Pending = 5,
		ContentServer = 6,
		Clan = 7,
		Chat = 8,
		ConsoleUser = 9,
		AnonUser = 10,
	};
}
