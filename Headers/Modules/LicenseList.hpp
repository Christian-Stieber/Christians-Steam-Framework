#pragma once

#include "MiscIDs.hpp"
#include "Printable.hpp"
#include "Steam/LicenseType.hpp"
#include "Steam/PaymentMethod.hpp"

#include <unordered_map>
#include <memory>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace LicenseList
        {
            namespace Whiteboard
            {
                class Licenses : public Printable
                {
                public:
                    // Note: the server sends more information, so we can add
                    // it if needed. I don't even need the items below, just
                    // added them to have something.
                    class LicenseInfo : public Printable
                    {
                    public:
                        LicenseInfo();
                        virtual ~LicenseInfo();

                    public:
                        PackageID packageId;
                        LicenseType licenseType=LicenseType::NoLicense;
                        PaymentMethod paymentMethod=PaymentMethod::None;

                    public:
                        virtual boost::json::value toJson() const override;
                    };

                public:
                    std::unordered_map<PackageID, std::shared_ptr<const LicenseInfo>> licenses;

                public:
                    Licenses();
                    virtual ~Licenses();

                public:
                    virtual boost::json::value toJson() const override;
                };
            }
        }
    }
}
