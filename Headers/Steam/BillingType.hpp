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

#include "Modules/PackageData.hpp"

/************************************************************************/
/*
 * SteamKit: SteamLanguage.cs -> EBillingType
 */

namespace SteamBot
{
	enum class BillingType
	{
        Unknown = -1,
        NoCost = 0,
        BillOnceOnly = 1,
        BillMonthly = 2,
        ProofOfPrepurchaseOnly = 3,
        GuestPass = 4,
        HardwarePromo = 5,
        Gift = 6,
        AutoGrant = 7,
        OEMTicket = 8,
        RecurringOption = 9,
        BillOnceOrCDKey = 10,
        Repurchaseable = 11,
        FreeOnDemand = 12,
        Rental = 13,
        CommercialLicense = 14,
        FreeCommercialLicense = 15,
        NumBillingTypes = 16
    };

    BillingType getBillingType(const Modules::PackageData::PackageInfo&);
}
