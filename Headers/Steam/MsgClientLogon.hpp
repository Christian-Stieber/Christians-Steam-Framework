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
 * SteamKit2/Base/Generated/SteamLanguageInternal.cs
 */

namespace Steam
{
	namespace MsgClientLogon
	{
		inline constexpr uint32_t ObfuscationMask = 0xBAADF00D;
		inline constexpr uint32_t CurrentProtocol = 65580;
		inline constexpr uint32_t ProtocolVerMajorMask = 0xFFFF0000;
		inline constexpr uint32_t ProtocolVerMinorMask = 0xFFFF;
		inline constexpr uint16_t ProtocolVerMinorMinGameServers = 4;
		inline constexpr uint16_t ProtocolVerMinorMinForSupportingEMsgMulti = 12;
		inline constexpr uint16_t ProtocolVerMinorMinForSupportingEMsgClientEncryptPct = 14;
		inline constexpr uint16_t ProtocolVerMinorMinForExtendedMsgHdr = 17;
		inline constexpr uint16_t ProtocolVerMinorMinForCellId = 18;
		inline constexpr uint16_t ProtocolVerMinorMinForSessionIDLast = 19;
		inline constexpr uint16_t ProtocolVerMinorMinForServerAvailablityMsgs = 24;
		inline constexpr uint16_t ProtocolVerMinorMinClients = 25;
		inline constexpr uint16_t ProtocolVerMinorMinForOSType = 26;
		inline constexpr uint16_t ProtocolVerMinorMinForCegApplyPESig = 27;
		inline constexpr uint16_t ProtocolVerMinorMinForMarketingMessages2 = 27;
		inline constexpr uint16_t ProtocolVerMinorMinForAnyProtoBufMessages = 28;
		inline constexpr uint16_t ProtocolVerMinorMinForProtoBufLoggedOffMessage = 28;
		inline constexpr uint16_t ProtocolVerMinorMinForProtoBufMultiMessages = 28;
		inline constexpr uint16_t ProtocolVerMinorMinForSendingProtocolToUFS = 30;
		inline constexpr uint16_t ProtocolVerMinorMinForMachineAuth = 33;
		inline constexpr uint16_t ProtocolVerMinorMinForSessionIDLastAnon = 36;
		inline constexpr uint16_t ProtocolVerMinorMinForEnhancedAppList = 40;
		inline constexpr uint16_t ProtocolVerMinorMinForSteamGuardNotificationUI = 41;
		inline constexpr uint16_t ProtocolVerMinorMinForProtoBufServiceModuleCalls = 42;
		inline constexpr uint16_t ProtocolVerMinorMinForGzipMultiMessages = 43;
		inline constexpr uint16_t ProtocolVerMinorMinForNewVoiceCallAuthorize = 44;
		inline constexpr uint16_t ProtocolVerMinorMinForClientInstanceIDs = 44;
	}
}
