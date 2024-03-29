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
                        uint64_t accessToken=0;
                        LicenseType licenseType=LicenseType::NoLicense;
                        PaymentMethod paymentMethod=PaymentMethod::None;
                        Clock::time_point timeCreated;
                        Clock::time_point timeNextProcess;

                    public:
                        virtual boost::json::value toJson() const override;
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
                    std::vector<SteamBot::PackageID> licenses;
                };
            }
        }
    }
}
