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
 * SteamKit2, SteamLanguage.cs, EPlatformType
 */

namespace Steam
{
    enum class PlatformType {
        Unknown = 0,
        Win32 = 1,
        Win64 = 2,
        Linux64 = 3,
        OSX = 4,
        PS3 = 5,
        Linux32 = 6,
    };
}

/************************************************************************/
/*
 * SteamKit2, SteamLanguage.cs, EOSType
 */

namespace Steam
{
    enum class OSType {
        Unknown = -1,
        Web = -700,
        IOSUnknown = -600,
        IOS1 = -599,
        IOS2 = -598,
        IOS3 = -597,
        IOS4 = -596,
        IOS5 = -595,
        IOS6 = -594,
        IOS6_1 = -593,
        IOS7 = -592,
        IOS7_1 = -591,
        IOS8 = -590,
        IOS8_1 = -589,
        IOS8_2 = -588,
        IOS8_3 = -587,
        IOS8_4 = -586,
        IOS9 = -585,
        IOS9_1 = -584,
        IOS9_2 = -583,
        IOS9_3 = -582,
        IOS10 = -581,
        IOS10_1 = -580,
        IOS10_2 = -579,
        IOS10_3 = -578,
        IOS11 = -577,
        IOS11_1 = -576,
        IOS11_2 = -575,
        IOS11_3 = -574,
        IOS11_4 = -573,
        IOS12 = -572,
        IOS12_1 = -571,
        AndroidUnknown = -500,
        Android6 = -499,
        Android7 = -498,
        Android8 = -497,
        Android9 = -496,
        UMQ = -400,
        PS3 = -300,
        MacOSUnknown = -102,
        MacOS104 = -101,
        MacOS105 = -100,
        MacOS1058 = -99,
        MacOS106 = -95,
        MacOS1063 = -94,
        MacOS1064_slgu = -93,
        MacOS1067 = -92,
        MacOS107 = -90,
        MacOS108 = -89,
        MacOS109 = -88,
        MacOS1010 = -87,
        MacOS1011 = -86,
        MacOS1012 = -85,
        Macos1013 = -84,
        Macos1014 = -83,
        Macos1015 = -82,
        MacOS1016 = -81,
        MacOS11 = -80,
        MacOS111 = -79,
        MacOS1017 = -78,
        MacOS12 = -77,
        MacOS13 = -76,
        MacOSMax = -1,
        LinuxUnknown = -203,
        Linux22 = -202,
        Linux24 = -201,
        Linux26 = -200,
        Linux32 = -199,
        Linux35 = -198,
        Linux36 = -197,
        Linux310 = -196,
        Linux316 = -195,
        Linux318 = -194,
        Linux3x = -193,
        Linux4x = -192,
        Linux41 = -191,
        Linux44 = -190,
        Linux49 = -189,
        Linux414 = -188,
        Linux419 = -187,
        Linux5x = -186,
        Linux54 = -185,
        Linux6x = -184,
        Linux7x = -183,
        Linux510 = -182,
        LinuxMax = -101,
        WinUnknown = 0,
        Win311 = 1,
        Win95 = 2,
        Win98 = 3,
        WinME = 4,
        WinNT = 5,
        Win2000 = 6,
        WinXP = 7,
        Win2003 = 8,
        WinVista = 9,
        Windows7 = 10,
        Win2008 = 11,
        Win2012 = 12,
        Windows8 = 13,
        Windows81 = 14,
        Win2012R2 = 15,
        Windows10 = 16,
        Win2016 = 17,
        Win2019 = 18,
        Win2022 = 19,
        Win11 = 20,
        WinMAX = 21,
    };

    OSType getOSType();
}

/************************************************************************/

namespace magic_enum
{
	namespace customize
	{
		template <typename T> struct enum_range;

		template <> struct enum_range<Steam::OSType>
		{
			static constexpr int min=-1000;
			static constexpr int max=100;
		};
	}
}
