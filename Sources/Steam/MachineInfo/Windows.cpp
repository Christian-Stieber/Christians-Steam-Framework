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

#ifdef _WIN32

#include "Steam/MachineInfo.hpp"

#include <cstring>

#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>

#include <Windows.h>
#include <WinReg.h>
#include <SysInfoAPI.h>
#include <FileAPI.h>

/************************************************************************/
/*
 * reg query HKLM\Software\Microsoft\Cryptography /v MachineGuid
 */

const boost::uuids::uuid& Steam::MachineInfo::Provider::getMachineGuid()
{
	class MachineGuid : public boost::uuids::uuid
	{
	public:
		MachineGuid()
		{
			char buffer[48];
			DWORD bufferSize=sizeof(buffer);
			auto result=RegGetValueA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography", "MachineGuid",
									 RRF_RT_REG_SZ, NULL, buffer, &bufferSize);
			if (result!=ERROR_SUCCESS)
			{
				throw std::runtime_error("RegGetValueA() failed for HKLM\\Software\\Microsoft\\Cryptography -> MachineGuid");
			}
			assert(bufferSize==36+1);

			auto uuid=boost::uuids::string_generator()(std::string(buffer));
			swap(uuid);
			BOOST_LOG_TRIVIAL(debug) << "machine-id is " << boost::uuids::to_string(*this);
		}
	};

	static const MachineGuid uuid;
	return uuid;
}

/************************************************************************/
/*
 * Right now, I'm too lazy to do anything useful here.
 */

const Steam::MachineInfo::EthernetAddress& Steam::MachineInfo::Provider::getMachineAddress()
{
	// ToDo: do some real work with GetAdaptersAddresses()

	// This showld be a locally administered unicast address
	static uint8_t fakeAddress[6]={ 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
	static const EthernetAddress address{fakeAddress};
	return address;
}

/************************************************************************/

const std::string& Steam::MachineInfo::Provider::getDiskId()
{
	static const std::string diskId=[]() -> std::string {
		char windowsDirectryPath[MAX_PATH];

		auto length=GetWindowsDirectoryA(windowsDirectryPath, sizeof(windowsDirectryPath));
		if (length==0)
		{
			throw std::runtime_error("GetWindowsDirectoryA() failed");
		}
		if (length>=sizeof(windowsDirectryPath))
		{
			// yes, I'm lazy
			throw std::runtime_error("GetWindowsDirectoryA() -- path too long");
		}

		assert(windowsDirectryPath[0]>='A' && windowsDirectryPath[0]<='Z');
		assert(windowsDirectryPath[1]==':');
		assert(windowsDirectryPath[2]=='\\');
		windowsDirectryPath[3]='\0';

		char volumeName[60];
		if (GetVolumeNameForVolumeMountPointA(windowsDirectryPath, volumeName, sizeof(volumeName))==0)
		{
			throw std::runtime_error("GetVolumeNameForVolumeMountPointA() failed");
		}

        // "\\?\Volume{963b00fb-9409-4c1e-9b82-5560aec8032d}\"
        static const char volumeNamePrefix[] = "\\\\?\\Volume{";
        if (memcmp(volumeName, volumeNamePrefix, strlen(volumeNamePrefix))!=0 || volumeName[47]!='}')
        {
            throw std::runtime_error("GetVolumeNameForVolumeMountPointA() -- unexpected result string");
        }

		volumeName[47]='\0';
		auto result=std::string(volumeName+strlen(volumeNamePrefix));
        BOOST_LOG_TRIVIAL(debug) << "disk-id is " << result;
        return result;
	}();
	return diskId;
}

/************************************************************************/

#endif /* _WIN32 */
