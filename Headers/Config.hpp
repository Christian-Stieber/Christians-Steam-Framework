#pragma once

#include <string>

/************************************************************************/

namespace SteamBot
{
	namespace Config
	{
		class SteamAccount
		{
		public:
			const std::string user;
			const std::string password;

			static const SteamAccount& get();

		private:
			SteamAccount();

		public:
			~SteamAccount();
		};
	}
}
