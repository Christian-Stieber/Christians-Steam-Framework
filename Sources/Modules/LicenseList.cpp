#include "Client/Module.hpp"
#include "Modules/LicenseList.hpp"
#include "Steam/ProtoBuf/steammessages_clientserver.hpp"
#include "EnumString.hpp"

/************************************************************************/

typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

/************************************************************************/

namespace
{
    class LicenseListModule : public SteamBot::Client::Module
    {
    private:
        void handleMessage(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType>);

    public:
        LicenseListModule() =default;
        virtual ~LicenseListModule() =default;

        virtual void run() override;
    };

    LicenseListModule::Init<LicenseListModule> init;
}

/************************************************************************/

Licenses::Licenses() =default;
Licenses::~Licenses() =default;

Licenses::LicenseInfo::LicenseInfo() =default;
Licenses::LicenseInfo::~LicenseInfo() =default;

/************************************************************************/

boost::json::value Licenses::LicenseInfo::toJson() const
{
    boost::json::object json;
    SteamBot::enumToJson(json, "packageId", packageId);
    SteamBot::enumToJson(json, "licenseType", licenseType);
    SteamBot::enumToJson(json, "paymentMethod", paymentMethod);
    return json;
}

/************************************************************************/

boost::json::value Licenses::toJson() const
{
    boost::json::array json;
    for (const auto& item : licenses)
    {
        json.push_back(item.second->toJson());
    }
    return json;
}

/************************************************************************/

void LicenseListModule::handleMessage(std::shared_ptr<const Steam::CMsgClientLicenseListMessageType> message)
{
    Licenses licenses;

    for (int index=0; index<message->content.licenses_size(); index++)
    {
        const auto& licenseData=message->content.licenses(index);
        if (licenseData.has_package_id())
        {
            auto license=std::make_shared<Licenses::LicenseInfo>();
            license->packageId=static_cast<SteamBot::PackageID>(licenseData.package_id());
            if (licenseData.has_license_type()) license->licenseType=static_cast<SteamBot::LicenseType>(licenseData.license_type());
            if (licenseData.has_payment_method()) license->paymentMethod=static_cast<SteamBot::PaymentMethod>(licenseData.payment_method());

            bool success=licenses.licenses.try_emplace(license->packageId, std::move(license)).second;
            assert(success);
        }
    }

    getClient().whiteboard.set(std::move(licenses));
}

/************************************************************************/

void LicenseListModule::run()
{
    getClient().launchFiber("OwnedGamesModule::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<Steam::CMsgClientLicenseListMessageType>> cmsgClientLicenseList;
        cmsgClientLicenseList=waiter.createWaiter<decltype(cmsgClientLicenseList)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter.wait();
            handleMessage(cmsgClientLicenseList->fetch());
            BOOST_LOG_TRIVIAL(info) << "license list: " << *(getClient().whiteboard.has<Licenses>());
        }
    });
}
