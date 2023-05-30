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

#include <filesystem>

/************************************************************************/

#ifdef __linux__
#include <unistd.h>
#include <pwd.h>
#endif

/************************************************************************/

static const std::filesystem::path& getHomeDir()
{
	static const std::filesystem::path homeDir=[](){
#ifdef __linux__
		// I don't use getpwuid_r(), since this is only meant to be called
		// once on startup.
		const struct passwd* entry=getpwuid(getuid());
		if (entry==nullptr)
		{
			int Errno=errno;
			throw std::system_error(Errno, std::generic_category());
		}
		return entry->pw_dir;
#else
#error Missing implementation for platform
#endif
	}();
	return homeDir;
}

/************************************************************************/

void SteamBot::setWorkingDir()
{
	std::filesystem::current_path(getHomeDir() / ".SteamBot");
}
