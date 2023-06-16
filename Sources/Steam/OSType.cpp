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

#include <boost/log/trivial.hpp>
#include <string.h>

#include "Steam/OSType.hpp"
#include "EnumString.hpp"

/************************************************************************/

#ifdef __linux__
static Steam::OSType getOSType_()
{
	/* I have no idea what the OSType-values are good for, so... */
	return Steam::OSType::LinuxUnknown;
}
#endif

/************************************************************************/

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>

static Steam::OSType getOSType_()
{
	char buffer[256];
	size_t length=sizeof(buffer)-1;
	if (sysctlbyname("kern.osproductversion", buffer, &length, nullptr, 0)==0)
	{
		assert(length<sizeof(buffer));
		buffer[length]='\0';

		static const struct
		{
			const char* prefix;
			OSType code;
		} versions[]=
		{
			{ "10.7", Steam::OSType::MacOS107 },
			{ "10.8", Steam::OSType::MacOS108 },
			{ "10.9", Steam::OSType::MacOS109 },
			{ "10.10", Steam::OSType::MacOS1010 },
			{ "10.11", Steam::OSType::MacOS1011 },
			{ "10.12", Steam::OSType::MacOS1012 },
			{ "10.13", Steam::OSType::Macos1013 },
			{ "10.14", Steam::OSType::Macos1014 },
			{ "10.15", Steam::OSType::Macos1015 },
			{ "11", Steam::OSType::MacOS11 },
			{ "12", Steam::OSType::MacOS12 }
		};

		for (const auto& entry: versions)
		{
			const char* system=buffer;
			const char* prefix=entry.prefix;
			while (*prefix==*system)
			{
				if (*prefix=='\0')
				{
					// precise match
					return entry.code;
				}
				prefix++;
				system++;
			}
			if (*prefix=='\0' && *system=='.')
			{
				// system has more components, like comparing "10.15" with "10.15.1"
				return entry.code;
			}
		}
		BOOST_LOG_TRIVIAL(info) << "unsupported operating system version: \"" << buffer << "\"";
	}
	else
	{
		const int myErrno=errno;
		BOOST_LOG_TRIVIAL(error) << "unable to get operating system version: " << strerror(myErrno);
	}
	return OSType::MacOSUnknown;
}
#endif

/************************************************************************/

#ifdef _WIN32
#include <windows.h>
#include <versionhelpers.h>
Steam::OSType getOSType_()
{
	// if (IsWindows11OrGreater()) return Steam::OSType::Win11;
	if (IsWindows10OrGreater()) return Steam::OSType::Windows10;
	if (IsWindows8Point1OrGreater()) return Steam::OSType::Windows81;
	if (IsWindows8OrGreater()) return Steam::OSType::Windows8;
	if (IsWindows7OrGreater()) return Steam::OSType::Windows7;
	if (IsWindowsVistaOrGreater()) return Steam::OSType::WinVista;
	if (IsWindowsVersionOrGreater(5,2,0)) return Steam::OSType::Win2003;
	if (IsWindowsXPOrGreater()) return Steam::OSType::WinXP;
	return Steam::OSType::WinUnknown;
}
#endif

/************************************************************************/
/*
 * SteamKit2, Utils.cs, GetOSType()
 */

Steam::OSType Steam::getOSType()
{
	static const OSType osType=[](){
		auto result=getOSType_();
        auto resultValue=static_cast<std::underlying_type_t<decltype(result)>>(result);
		BOOST_LOG_TRIVIAL(info) << "OS type: " << SteamBot::enumToStringAlways(result) << " (" << resultValue << ")";
		return result;
	}();

	return osType;
}
