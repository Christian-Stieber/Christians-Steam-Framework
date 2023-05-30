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
