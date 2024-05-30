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

#include "Settings.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace CardFarmer
        {
            void use();
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace CardFarmer
        {
            namespace Settings
            {
                class Enable : public SteamBot::Settings::SettingBool
                {
                public:
                    using SettingBool::SettingBool;

                private:
                    virtual const std::string_view& name() const override;
                    virtual void storeWhiteboard(Ptr<>) const override;
                };
            }
        }
    }
}
