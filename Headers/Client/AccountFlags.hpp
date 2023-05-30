#pragma once

#include "EnumString.hpp"

/************************************************************************/
/*
 * SteamKit2 "EAccountFlags"
 */

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		enum AccountFlags : uint32_t
		{
			NormalUser = 0,
			PersonaNameSet = (1<<0),
			Unbannable = (1<<1),
			PasswordSet = (1<<2),
			Support = (1<<3),
			Admin = (1<<4),
			Supervisor = (1<<5),
			AppEditor = (1<<6),
			HWIDSet = (1<<7),
			PersonalQASet = (1<<8),
			VacBeta = (1<<9),
			Debug = (1<<10),
			Disabled = (1<<11),
			LimitedUser = (1<<12),
			LimitedUserForce = (1<<13),
			EmailValidated = (1<<14),
			MarketingTreatment = (1<<15),
			OGGInviteOptOut = (1<<16),
			ForcePasswordChange = (1<<17),
			ForceEmailVerification = (1<<18),
			LogonExtraSecurity = (1<<19),
			LogonExtraSecurityDisabled = (1<<20),
			Steam2MigrationComplete = (1<<21),
			NeedLogs = (1<<22),
			Lockdown = (1<<23),
			MasterAppEditor = (1<<24),
			BannedFromWebAPI = (1<<25),
			ClansOnlyFromFriends = (1<<26),
			GlobalModerator = (1<<27),
			ParentalSettings = (1<<28),
			ThirdPartySupport = (1<<29),
			NeedsSSANextSteamLogon = (1<<30),
		};
	}
}

/************************************************************************/

namespace magic_enum
{
	namespace customize
	{
		template <> struct enum_range<Steam::Client::AccountFlags>
		{
			static constexpr bool is_flags=true;
		};
	}
}
