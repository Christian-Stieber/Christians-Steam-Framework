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

namespace SteamBot
{
    namespace Modules
    {
        namespace LicenseList
        {
            namespace Whiteboard
            {
                // Note: the whiteboard actually holds a Licenses::Ptr!!!!

                class Licenses : public Printable
                {
                public:
                    typedef std::shared_ptr<Licenses> Ptr;

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
