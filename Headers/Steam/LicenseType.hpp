#pragma once

/************************************************************************/
/*
 * SteamKit: SteamLanguage.cs -> ELicenseType
 */

namespace SteamBot
{
	enum class LicenseType : uint32_t
	{
		NoLicense = 0,
		SinglePurchase = 1,
		SinglePurchaseLimitedUse = 2,
		RecurringCharge = 3,
		RecurringChargeLimitedUse = 4,
		RecurringChargeLimitedUseWithOverages = 5,
		RecurringOption = 6,
		LimitedUseDelayedActivation = 7,
	};
}

