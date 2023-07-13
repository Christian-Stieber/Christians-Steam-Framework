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

#include "WorkingDir.hpp"

/************************************************************************/

#if defined(__linux__)

#include <filesystem>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

void SteamBot::setWorkingDir()
{
    umask(S_IXOTH | S_IWOTH | S_IROTH | S_IXGRP | S_IWGRP | S_IRGRP);

	static const std::filesystem::path homeDir=[](){
		// I don't use getpwuid_r(), since this is only meant to be called
		// once on startup.
		const struct passwd* entry=getpwuid(getuid());
		if (entry==nullptr)
		{
			int Errno=errno;
			throw std::system_error(Errno, std::generic_category());
		}
		return entry->pw_dir;
	}();

    auto dirPath=(homeDir / ".Christians-Steam-Framework");
    std::filesystem::create_directory(dirPath);
    std::filesystem::current_path(dirPath);
}

/************************************************************************/

#elif defined(_WIN32)

#include <string>
#include <cassert>

#include <windows.h>
#include <shlobj_core.h>

void SteamBot::setWorkingDir()
{
	std::wstring path(L"\\\\?\\");

	{
		PWSTR folderPath=nullptr;
		{
			auto result=SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &folderPath);
			assert(result==S_OK);
		}
		path.append(folderPath);
		CoTaskMemFree(folderPath);
	}

	path.append(L"\\Christian-Stieber");
	if (CreateDirectoryW(path.c_str(), NULL)==0)
	{
		assert(GetLastError()==ERROR_ALREADY_EXISTS);
	}

	path.append(L"\\Steam-Framework");
	if (CreateDirectoryW(path.c_str(), NULL)==0)
	{
		assert(GetLastError()==ERROR_ALREADY_EXISTS);
	}

	path.append(L"\\");

	{
		auto result=SetCurrentDirectoryW(path.c_str());
		assert(result);
	}
}

/************************************************************************/

#else
#error Missing implementation for platform
#endif
