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

#include "MiscIDs.hpp"
#include "Printable.hpp"
#include "Steam/LicenseType.hpp"
#include "Steam/PaymentMethod.hpp"

#include <unordered_map>
#include <unordered_set>
#include <memory>

/************************************************************************/

class CMsgClientLicenseList_License;		// protobuf

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace LicenseList
        {
            namespace Whiteboard
            {
                class LicenseIdentifier : public Printable
                {
                public:
                    PackageID packageId;
                    uint32_t changeNumber=0;

                public:
                    LicenseIdentifier(PackageID);
                    LicenseIdentifier(const LicenseIdentifier&);
                    LicenseIdentifier(const boost::json::value&);

                    LicenseIdentifier& operator=(const LicenseIdentifier&);
                    constexpr bool operator==(const LicenseIdentifier&) const =default;

                    virtual ~LicenseIdentifier();
                    virtual boost::json::value toJson() const override;
                };

                // Note: the whiteboard actually holds a Licenses::Ptr!!!!
                class Licenses : public Printable
                {
                public:
                    typedef std::shared_ptr<const Licenses> Ptr;

                public:
                    // Note: the server sends more information, so we can add
                    // it if needed. I don't even need the items below, just
                    // added them to have something.
                    class LicenseInfo : public LicenseIdentifier
                    {
                    public:
                        typedef std::chrono::system_clock Clock;

                    public:
                        LicenseInfo(const CMsgClientLicenseList_License&);
                        virtual ~LicenseInfo();

                    public:
                        // SteamKit, enum ELicenseFlags
                        enum class Flags : uint32_t {
                            None = 0,
                            Renew = 0x01,
                            RenewalFailed = 0x02,
                            Pending = 0x04,
                            Expired = 0x08,
                            CancelledByUser = 0x10,
                            CancelledByAdmin = 0x20,
                            LowViolenceContent = 0x40,
                            ImportedFromSteam2 = 0x80,
                            ForceRunRestriction = 0x100,
                            RegionRestrictionExpired = 0x200,
                            CancelledByFriendlyFraudLock = 0x400,
                            NotActivated = 0x800,
                            PendingRefund = 0x2000,
                            Borrowed = 0x4000,
                            ReleaseStateOverride = 0x8000,
                            CancelledByPartner = 0x40000,
                            NonPermanent = 0x80000,
                            PreferredOwner = 0x100000
                        };

                    public:
                        uint64_t accessToken=0;
                        LicenseType licenseType=LicenseType::NoLicense;
                        PaymentMethod paymentMethod=PaymentMethod::None;
                        Clock::time_point timeCreated;
                        Clock::time_point timeNextProcess;
                        uint32_t flags=0;

                    public:
                        virtual boost::json::value toJson() const override;

                        bool isExpired() const
                        {
                           return flags&toInteger(Flags::Expired);
                        }
                    };

                public:
                    std::unordered_map<PackageID, std::shared_ptr<const LicenseInfo>> licenses;

                public:
                    Licenses();
                    virtual ~Licenses();

                    std::shared_ptr<const Licenses::LicenseInfo> getInfo(PackageID) const;

                public:
                    virtual boost::json::value toJson() const override;
                };
            }

            std::shared_ptr<const Whiteboard::Licenses::LicenseInfo> getLicenseInfo(PackageID);
        }
    }
}

/************************************************************************/
/*
 * The whiteboard will get the Licenses::Ptr before we send this
 * message.
 *
 * Also, this will be sent whenever we get a new license list, even
 * if nothing was added.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace LicenseList
        {
            namespace Messageboard
            {
                class NewLicenses
                {
                public:
                    std::unordered_set<SteamBot::PackageID> licenses;
                };
            }
        }
    }
}
