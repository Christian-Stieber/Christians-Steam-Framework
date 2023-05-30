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
