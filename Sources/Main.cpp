#include "WorkingDir.hpp"
#include "Client/Client.hpp"

#include <locale>

/************************************************************************/

int main(void)
{
	std::locale::global(std::locale::classic());
	SteamBot::setWorkingDir();

    SteamBot::Client::launch();
    SteamBot::Client::waitAll();

	return EXIT_SUCCESS;
}
