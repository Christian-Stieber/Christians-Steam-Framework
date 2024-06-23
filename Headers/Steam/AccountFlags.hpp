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

/************************************************************************/
/*
 * SteamKit2, SteamLanguage.cs, EAccountFlags
 */

namespace Steam
{
    enum class AccountFlags : uint32_t {
        NormalUser = 0,
        PersonaNameSet = 1,
        Unbannable = 2,
        PasswordSet = 4,
        Support = 8,
        Admin = 16,
        Supervisor = 32,
        AppEditor = 64,
        HWIDSet = 128,
        PersonalQASet = 256,
        VacBeta = 512,
        Debug = 1024,
        Disabled = 2048,
        LimitedUser = 4096,
        LimitedUserForce = 8192,
        EmailValidated = 16384,
        MarketingTreatment = 32768,
        OGGInviteOptOut = 65536,
        ForcePasswordChange = 131072,
        ForceEmailVerification = 262144,
        LogonExtraSecurity = 524288,
        LogonExtraSecurityDisabled = 1048576,
        Steam2MigrationComplete = 2097152,
        NeedLogs = 4194304,
        Lockdown = 8388608,
        MasterAppEditor = 16777216,
        BannedFromWebAPI = 33554432,
        ClansOnlyFromFriends = 67108864,
        GlobalModerator = 134217728,
        ParentalSettings = 268435456,
        ThirdPartySupport = 536870912,
        NeedsSSANextSteamLogon = 1073741824
    };
}

/************************************************************************/

#include "External/MagicEnum/magic_enum.hpp"

template <> struct magic_enum::customize::enum_range<Steam::AccountFlags>
{
    static constexpr bool is_flags = true;
};
