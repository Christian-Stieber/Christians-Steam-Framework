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

#include "Client/Client.hpp"
#include "Client/LicenseList.hpp"
#include "Steam/LicenseType.hpp"
#include "Steam/PaymentMethod.hpp"
#include "UI/UI.hpp"
#include "EnumString.hpp"

/************************************************************************/

Steam::Client::LicenseList::LicenseList(Steam::Client::Client& client)
{
	client.getDispatcher().setHandler<CMsgClientLicenseListMessageType>(this);
}

/************************************************************************/

Steam::Client::LicenseList::~LicenseList() =default;

/************************************************************************/

void Steam::Client::LicenseList::handleMessage(boost::asio::yield_context yield, std::shared_ptr<CMsgClientLicenseListMessageType> message)
{
	if (static_cast<ResultCode>(message->content.eresult())==ResultCode::OK)
	{
		SteamBot::UI::Handle ui(yield);
		ui.print() << "licenses:\n";
		for (int i=0; i<message->content.licenses_size(); i++)
		{
			const auto& license=message->content.licenses(i);
			{
				const auto licenseType=static_cast<Steam::LicenseType>(license.license_type());
				ui.print() << "   type: " << SteamBot::enumToStringAlways(licenseType) << "\n";
			}
			{
				const auto paymentMethod=static_cast<Steam::LicenseType>(license.payment_method());
				ui.print() << "   payment: " << SteamBot::enumToStringAlways(paymentMethod) << "\n";
			}
		}
	}
}
