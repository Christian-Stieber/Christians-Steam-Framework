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
